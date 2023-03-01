// Check implicit "acc routine" directives not covered by routine-implicit.c
// because they are specific to C++.
//
// Implicit routine directives for C++ class/namespace member functions are
// checked as part of routine-placement-for-class.cpp and
// routine-placement-for-namespace.cpp.
//
// FIXME: We skip execution checks for source-to-source mode because AST
// printing for templates includes separate printing of the template
// specializations, which breaks the compilation.

// REDEFINE: %{dmp:fc:args} = -strict-whitespace
// REDEFINE: %{exe:fc:args} = -strict-whitespace -match-full-lines \
// REDEFINE:                  -implicit-check-not='{{.}}'
// RUN: %{acc-check-dmp-cxx}
// RUN: %{acc-check-prt-cxx}
// RUN: %{acc-check-exe-cxx-no-s2s}

// END.

/* expected-no-diagnostics */

#include <stdio.h>

// EXE-NOT:{{.}}

#define A_SIZE 4
#define NUM_GANGS 4

// Each of these functions has an (unnecessary) routine directive.  The point is
// that should not imply a routine directive for any lambda calling it.
#pragma acc routine seq
void initArray(int *a) {
  for (int i = 0; i < A_SIZE; ++i)
    a[i] = i;
}
#pragma acc routine seq
void printArray(int *a) {
  for (int i = 0; i < A_SIZE; ++i)
    printf("a[%d]=%d\n", i, a[i]);
}

template <typename Lambda>
void callLambdaInParallel(const char *title, int ngangs, Lambda lambda) {
  printf("%s\n", title);
  int a[A_SIZE];
  initArray(a);
  #pragma acc parallel num_gangs(ngangs)
  lambda(a);
  printArray(a);
}

template <typename Lambda>
void callLambdaFromHost(const char *title, int ngangs, Lambda lambda) {
  printf("%s\n", title);
  lambda(ngangs);
}

#pragma acc routine gang
template <typename Lambda>
void gangFnCallingLambda(int *a, Lambda lambda) { lambda(a); }

#pragma acc routine worker
template <typename Lambda>
void workerFnCallingLambda(int *a, Lambda lambda) { lambda(a); }

#pragma acc routine vector
template <typename Lambda>
void vectorFnCallingLambda(int *a, Lambda lambda) { lambda(a); }

#pragma acc routine seq
template <typename Lambda>
void seqFnCallingLambda(int *a, Lambda lambda) { lambda(a); }

#define LOOP                       \
  for (int i = 0; i < A_SIZE; ++i) \
    a[i] += 10;

#pragma acc routine gang
void gangFn(int *a) {
  #pragma acc loop gang
  LOOP
}

#pragma acc routine worker
void workerFn(int *a) {
  #pragma acc loop worker
  LOOP
}

#pragma acc routine vector
void vectorFn(int *a) {
  #pragma acc loop vector
  LOOP
}

#pragma acc routine seq
void seqFn(int *a) {
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
void impSeqFn(int *a) {
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
//   EXE-NOT:{{.}}
void checkGangFnCallInLambda() {
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
//   EXE-NOT:{{.}}
void checkWorkerFnCallInLambda() {
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
//   EXE-NOT:{{.}}
void checkVectorFnCallInLambda() {
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
//   EXE-NOT:{{.}}
void checkSeqFnCallInLambda() {
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
//   EXE-NOT:{{.}}
void checkImpSeqFnCallInLambda() {
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
//   EXE-NOT:{{.}}
void checkSeqFnCallInHostLambda() {
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
//------------------------------------------------------------------------------

// DMP-LABEL: FunctionDecl {{.*}} checkGangLoopInLambda 'void ()'
//       DMP:   LambdaExpr
//  DMP-NEXT:     CXXRecordDecl
//       DMP:       CXXMethodDecl
//  DMP-NEXT:         ParmVarDecl
//  DMP-NEXT:         CompoundStmt
//       DMP:           ACCLoopDirective
//  DMP-NEXT:             ACCGangClause
//  DMP-NEXT:             ACCIndependentClause {{.*}} <implicit>
//  DMP-NEXT:             ACCSharedClause {{.*}} <implicit>
//  DMP-NEXT:               DeclRefExpr {{.*}} 'a'
//  DMP-NEXT:             impl: OMPDistributeDirective
//   DMP-NOT:               OMP
//       DMP:               ForStmt
//       DMP:         ACCRoutineDeclAttr {{.*}} Implicit Gang OMPNodeKind=unknown DirectiveDiscardedForOMP
//
//   PRT-LABEL: void checkGangLoopInLambda() {
//    PRT-NEXT:   callLambdaInParallel("{{.*}}", {{.*}}, [](int *a) {
//  PRT-A-NEXT:     {{^ *}}#pragma acc loop gang{{$}}
// PRT-AO-NEXT:     {{^ *}}// #pragma omp distribute{{$}}
//  PRT-O-NEXT:     {{^ *}}#pragma omp distribute{{$}}
// PRT-OA-NEXT:     {{^ *}}// #pragma acc loop gang{{$}}
//         PRT:   });
//    PRT-NEXT: }
//
// EXE-LABEL:checkGangLoopInLambda
//  EXE-NEXT:a[0]=10
//  EXE-NEXT:a[1]=11
//  EXE-NEXT:a[2]=12
//  EXE-NEXT:a[3]=13
//   EXE-NOT:{{.}}
void checkGangLoopInLambda() {
  callLambdaInParallel("checkGangLoopInLambda", NUM_GANGS, [](int *a) {
    #pragma acc loop gang
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
//   EXE-NOT:{{.}}
void checkGangWorkerVectorLoopInLambda() {
  callLambdaInParallel("checkGangWorkerVectorLoopInLambda", NUM_GANGS, [](int *a) {
    #pragma acc loop vector gang worker
    LOOP
  });
}

// DMP-LABEL: FunctionDecl {{.*}} checkWorkerLoopInLambda 'void ()'
//       DMP:   LambdaExpr
//  DMP-NEXT:     CXXRecordDecl
//       DMP:       CXXMethodDecl
//  DMP-NEXT:         ParmVarDecl
//  DMP-NEXT:         CompoundStmt
//       DMP:           ACCLoopDirective
//  DMP-NEXT:             ACCWorkerClause
//  DMP-NEXT:             ACCIndependentClause {{.*}} <implicit>
//  DMP-NEXT:             ACCSharedClause {{.*}} <implicit>
//  DMP-NEXT:               DeclRefExpr {{.*}} 'a'
//  DMP-NEXT:             impl: OMPParallelForDirective
//   DMP-NOT:               OMP
//       DMP:               ForStmt
//       DMP:         ACCRoutineDeclAttr {{.*}} Implicit Worker OMPNodeKind=unknown DirectiveDiscardedForOMP
//
//   PRT-LABEL: void checkWorkerLoopInLambda() {
//    PRT-NEXT:   callLambdaInParallel("{{.*}}", 1, [](int *a) {
//  PRT-A-NEXT:     {{^ *}}#pragma acc loop worker{{$}}
// PRT-AO-NEXT:     {{^ *}}// #pragma omp parallel for{{$}}
//  PRT-O-NEXT:     {{^ *}}#pragma omp parallel for{{$}}
// PRT-OA-NEXT:     {{^ *}}// #pragma acc loop worker{{$}}
//         PRT:   });
//    PRT-NEXT: }
//
// EXE-LABEL:checkWorkerLoopInLambda
//  EXE-NEXT:a[0]=10
//  EXE-NEXT:a[1]=11
//  EXE-NEXT:a[2]=12
//  EXE-NEXT:a[3]=13
//   EXE-NOT:{{.}}
void checkWorkerLoopInLambda() {
  callLambdaInParallel("checkWorkerLoopInLambda", 1, [](int *a) {
    #pragma acc loop worker
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
//   EXE-NOT:{{.}}
void checkWorkerVectorLoopInLambda() {
  callLambdaInParallel("checkWorkerVectorLoopInLambda", 1, [](int *a) {
    #pragma acc loop worker vector
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
//   EXE-NOT:{{.}}
void checkVectorLoopInLambda() {
  callLambdaInParallel("checkVectorLoopInLambda", 1, [](int *a) {
    #pragma acc loop vector
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
//   EXE-NOT:{{.}}
void checkSeqLoopInLambda() {
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
//   EXE-NOT:{{.}}
void checkNakedLoopInLambda() {
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
//   EXE-NOT:{{.}}
void checkAutoLoopInLambda() {
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
//   EXE-NOT:{{.}}
void checkAutoGangWorkerVectorLoopInLambda() {
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
//   EXE-NOT:{{.}}
void checkSerialLoopInLambda() {
  callLambdaInParallel("checkSerialLoopInLambda", 1, [](int *a) {
    LOOP
  });
}

//------------------------------------------------------------------------------
// Check with multiple impliers.  First one of highest level should be
// identified.
//
// FIXME: Implicit gang clauses are not yet computed in lambdas with routine
// directives implied by the lambda body.  When they are, atomics will no longer
// be needed in loops below, and we can just use LOOP like everywhere else.
//------------------------------------------------------------------------------

// DMP-LABEL: FunctionDecl {{.*}} checkHighLoopLowLoopInLambda 'void ()'
//       DMP:   LambdaExpr
//  DMP-NEXT:     CXXRecordDecl
//       DMP:       CXXMethodDecl
//  DMP-NEXT:         ParmVarDecl
//  DMP-NEXT:         CompoundStmt
//       DMP:           ACCLoopDirective
//  DMP-NEXT:             ACCGangClause
//  DMP-NEXT:             ACCIndependentClause {{.*}} <implicit>
//  DMP-NEXT:             ACCSharedClause {{.*}} <implicit>
//  DMP-NEXT:               DeclRefExpr {{.*}} 'a'
//  DMP-NEXT:             impl: OMPDistributeDirective
//   DMP-NOT:               OMP
//       DMP:               ForStmt
//       DMP:           ACCLoopDirective
//  DMP-NEXT:             ACCVectorClause
//  DMP-NEXT:             ACCIndependentClause {{.*}} <implicit>
//  DMP-NEXT:             ACCSharedClause {{.*}} <implicit>
//  DMP-NEXT:               DeclRefExpr {{.*}} 'a'
//  DMP-NEXT:             impl: OMPSimdDirective
//   DMP-NOT:               OMP
//       DMP:               ForStmt
//       DMP:         ACCRoutineDeclAttr {{.*}} Implicit Gang OMPNodeKind=unknown DirectiveDiscardedForOMP
//
//   PRT-LABEL: void checkHighLoopLowLoopInLambda() {
//    PRT-NEXT:   callLambdaInParallel("{{.*}}", {{.*}}, [](int *a) {
//  PRT-A-NEXT:     {{^ *}}#pragma acc loop gang{{$}}
// PRT-AO-NEXT:     {{^ *}}// #pragma omp distribute{{$}}
//  PRT-O-NEXT:     {{^ *}}#pragma omp distribute{{$}}
// PRT-OA-NEXT:     {{^ *}}// #pragma acc loop gang{{$}}
//    PRT-NEXT:     {{LOOP|for \(.*\)[[:space:]].*;$}}
//  PRT-A-NEXT:     {{^ *}}#pragma acc loop vector{{$}}
// PRT-AO-NEXT:     {{^ *}}// #pragma omp simd{{$}}
//  PRT-O-NEXT:     {{^ *}}#pragma omp simd{{$}}
// PRT-OA-NEXT:     {{^ *}}// #pragma acc loop vector{{$}}
//    PRT-NEXT:     {{LOOP|for \(.*\) {([^{}]|[[:space:]])*}$}}
//    PRT-NEXT:   });
//    PRT-NEXT: }
//
// EXE-LABEL:checkHighLoopLowLoopInLambda
//  EXE-NEXT:a[0]=50
//  EXE-NEXT:a[1]=51
//  EXE-NEXT:a[2]=52
//  EXE-NEXT:a[3]=53
//   EXE-NOT:{{.}}
void checkHighLoopLowLoopInLambda() {
  callLambdaInParallel("checkHighLoopLowLoopInLambda", NUM_GANGS, [](int *a) {
    #pragma acc loop gang
    LOOP
    #pragma acc loop vector
    for (int i = 0; i < A_SIZE; ++i) {
      #pragma acc atomic update
      a[i] += 10;
    }
  });
}

// DMP-LABEL: FunctionDecl {{.*}} checkLowLoopHighLoopInLambda 'void ()'
//       DMP:   LambdaExpr
//  DMP-NEXT:     CXXRecordDecl
//       DMP:       CXXMethodDecl
//  DMP-NEXT:         ParmVarDecl
//  DMP-NEXT:         CompoundStmt
//       DMP:           ACCLoopDirective
//  DMP-NEXT:             ACCVectorClause
//  DMP-NEXT:             ACCIndependentClause {{.*}} <implicit>
//  DMP-NEXT:             ACCSharedClause {{.*}} <implicit>
//  DMP-NEXT:               DeclRefExpr {{.*}} 'a'
//  DMP-NEXT:             impl: OMPSimdDirective
//   DMP-NOT:               OMP
//       DMP:               ForStmt
//       DMP:           ACCLoopDirective
//  DMP-NEXT:             ACCGangClause
//  DMP-NEXT:             ACCIndependentClause {{.*}} <implicit>
//  DMP-NEXT:             ACCSharedClause {{.*}} <implicit>
//  DMP-NEXT:               DeclRefExpr {{.*}} 'a'
//  DMP-NEXT:             impl: OMPDistributeDirective
//   DMP-NOT:               OMP
//       DMP:               ForStmt
//       DMP:         ACCRoutineDeclAttr {{.*}} Implicit Gang OMPNodeKind=unknown DirectiveDiscardedForOMP
//
//   PRT-LABEL: void checkLowLoopHighLoopInLambda() {
//    PRT-NEXT:   callLambdaInParallel("{{.*}}", {{.*}}, [](int *a) {
//  PRT-A-NEXT:     {{^ *}}#pragma acc loop vector{{$}}
// PRT-AO-NEXT:     {{^ *}}// #pragma omp simd{{$}}
//  PRT-O-NEXT:     {{^ *}}#pragma omp simd{{$}}
// PRT-OA-NEXT:     {{^ *}}// #pragma acc loop vector{{$}}
//    PRT-NEXT:     {{LOOP|for \(.*\) {([^{}]|[[:space:]])*}$}}
//  PRT-A-NEXT:     {{^ *}}#pragma acc loop gang{{$}}
// PRT-AO-NEXT:     {{^ *}}// #pragma omp distribute{{$}}
//  PRT-O-NEXT:     {{^ *}}#pragma omp distribute{{$}}
// PRT-OA-NEXT:     {{^ *}}// #pragma acc loop gang{{$}}
//    PRT-NEXT:     {{LOOP|for \(.*\)[[:space:]].*;$}}
//    PRT-NEXT:   });
//    PRT-NEXT: }
//
// EXE-LABEL:checkLowLoopHighLoopInLambda
//  EXE-NEXT:a[0]=50
//  EXE-NEXT:a[1]=51
//  EXE-NEXT:a[2]=52
//  EXE-NEXT:a[3]=53
//   EXE-NOT:{{.}}
void checkLowLoopHighLoopInLambda() {
  callLambdaInParallel("checkLowLoopHighLoopInLambda", NUM_GANGS, [](int *a) {
    #pragma acc loop vector
    for (int i = 0; i < A_SIZE; ++i) {
      #pragma acc atomic update
      a[i] += 10;
    }
    #pragma acc loop gang
    LOOP
  });
}

// DMP-LABEL: FunctionDecl {{.*}} checkHighCallLowCallInLambda 'void ()'
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
//   PRT-LABEL: void checkHighCallLowCallInLambda() {
//    PRT-NEXT:   callLambdaInParallel("{{.*}}", {{.*}}, [](int *a) {
//    PRT-NEXT:     workerFn(a);
//    PRT-NEXT:     vectorFn(a);
//    PRT-NEXT:   });
//    PRT-NEXT: }
//
// EXE-LABEL:checkHighCallLowCallInLambda
//  EXE-NEXT:a[0]=20
//  EXE-NEXT:a[1]=21
//  EXE-NEXT:a[2]=22
//  EXE-NEXT:a[3]=23
//   EXE-NOT:{{.}}
void checkHighCallLowCallInLambda() {
  callLambdaInParallel("checkHighCallLowCallInLambda", 1, [](int *a) {
    workerFn(a);
    vectorFn(a);
  });
}

// DMP-LABEL: FunctionDecl {{.*}} checkLowCallHighCallInLambda 'void ()'
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
//   PRT-LABEL: void checkLowCallHighCallInLambda() {
//    PRT-NEXT:   callLambdaInParallel("{{.*}}", {{.*}}, [](int *a) {
//    PRT-NEXT:     vectorFn(a);
//    PRT-NEXT:     workerFn(a);
//    PRT-NEXT:   });
//    PRT-NEXT: }
//
// EXE-LABEL:checkLowCallHighCallInLambda
//  EXE-NEXT:a[0]=20
//  EXE-NEXT:a[1]=21
//  EXE-NEXT:a[2]=22
//  EXE-NEXT:a[3]=23
//   EXE-NOT:{{.}}
void checkLowCallHighCallInLambda() {
  callLambdaInParallel("checkLowCallHighCallInLambda", 1, [](int *a) {
    vectorFn(a);
    workerFn(a);
  });
}

// DMP-LABEL: FunctionDecl {{.*}} checkHighLoopLowCallInLambda 'void ()'
//       DMP:   LambdaExpr
//  DMP-NEXT:     CXXRecordDecl
//       DMP:       CXXMethodDecl
//  DMP-NEXT:         ParmVarDecl
//  DMP-NEXT:         CompoundStmt
//       DMP:           ACCLoopDirective
//  DMP-NEXT:             ACCGangClause
//  DMP-NEXT:             ACCIndependentClause {{.*}} <implicit>
//  DMP-NEXT:             ACCSharedClause {{.*}} <implicit>
//  DMP-NEXT:               DeclRefExpr {{.*}} 'a'
//  DMP-NEXT:             impl: OMPDistributeDirective
//   DMP-NOT:               OMP
//       DMP:               ForStmt
//       DMP:           CallExpr
//  DMP-NEXT:             ImplicitCastExpr
//  DMP-NEXT:               DeclRefExpr {{.*}} 'workerFn'
//       DMP:         ACCRoutineDeclAttr {{.*}} Implicit Gang OMPNodeKind=unknown DirectiveDiscardedForOMP
//
//   PRT-LABEL: void checkHighLoopLowCallInLambda() {
//    PRT-NEXT:   callLambdaInParallel("{{.*}}", {{.*}}, [](int *a) {
//  PRT-A-NEXT:     {{^ *}}#pragma acc loop gang{{$}}
// PRT-AO-NEXT:     {{^ *}}// #pragma omp distribute{{$}}
//  PRT-O-NEXT:     {{^ *}}#pragma omp distribute{{$}}
// PRT-OA-NEXT:     {{^ *}}// #pragma acc loop gang{{$}}
//    PRT-NEXT:     {{LOOP|for \(.*\)[[:space:]].*;$}}
//    PRT-NEXT:     workerFn(a);
//    PRT-NEXT:   });
//    PRT-NEXT: }
//
// EXE-LABEL:checkHighLoopLowCallInLambda
//  EXE-NEXT:a[0]=20
//  EXE-NEXT:a[1]=21
//  EXE-NEXT:a[2]=22
//  EXE-NEXT:a[3]=23
//   EXE-NOT:{{.}}
void checkHighLoopLowCallInLambda() {
  callLambdaInParallel("checkHighLoopLowCallInLambda", 1, [](int *a) {
    #pragma acc loop gang
    LOOP
    workerFn(a);
  });
}

// DMP-LABEL: FunctionDecl {{.*}} checkLowCallHighLoopInLambda 'void ()'
//       DMP:   LambdaExpr
//  DMP-NEXT:     CXXRecordDecl
//       DMP:       CXXMethodDecl
//  DMP-NEXT:         ParmVarDecl
//  DMP-NEXT:         CompoundStmt
//       DMP:           CallExpr
//  DMP-NEXT:             ImplicitCastExpr
//  DMP-NEXT:               DeclRefExpr {{.*}} 'workerFn'
//       DMP:           ACCLoopDirective
//  DMP-NEXT:             ACCGangClause
//  DMP-NEXT:             ACCIndependentClause {{.*}} <implicit>
//  DMP-NEXT:             ACCSharedClause {{.*}} <implicit>
//  DMP-NEXT:               DeclRefExpr {{.*}} 'a'
//  DMP-NEXT:             impl: OMPDistributeDirective
//   DMP-NOT:               OMP
//       DMP:               ForStmt
//       DMP:         ACCRoutineDeclAttr {{.*}} Implicit Gang OMPNodeKind=unknown DirectiveDiscardedForOMP
//
//   PRT-LABEL: void checkLowCallHighLoopInLambda() {
//    PRT-NEXT:   callLambdaInParallel("{{.*}}", {{.*}}, [](int *a) {
//    PRT-NEXT:     workerFn(a);
//  PRT-A-NEXT:     {{^ *}}#pragma acc loop gang{{$}}
// PRT-AO-NEXT:     {{^ *}}// #pragma omp distribute{{$}}
//  PRT-O-NEXT:     {{^ *}}#pragma omp distribute{{$}}
// PRT-OA-NEXT:     {{^ *}}// #pragma acc loop gang{{$}}
//    PRT-NEXT:     {{LOOP|for \(.*\)[[:space:]].*;$}}
//    PRT-NEXT:   });
//    PRT-NEXT: }
//
// EXE-LABEL:checkLowCallHighLoopInLambda
//  EXE-NEXT:a[0]=20
//  EXE-NEXT:a[1]=21
//  EXE-NEXT:a[2]=22
//  EXE-NEXT:a[3]=23
//   EXE-NOT:{{.}}
void checkLowCallHighLoopInLambda() {
  callLambdaInParallel("checkLowCallHighLoopInLambda", 1, [](int *a) {
    workerFn(a);
    #pragma acc loop gang
    LOOP
  });
}

// DMP-LABEL: FunctionDecl {{.*}} checkHighCallLowLoopInLambda 'void ()'
//       DMP:   LambdaExpr
//  DMP-NEXT:     CXXRecordDecl
//       DMP:       CXXMethodDecl
//  DMP-NEXT:         ParmVarDecl
//  DMP-NEXT:         CompoundStmt
//       DMP:           CallExpr
//  DMP-NEXT:             ImplicitCastExpr
//  DMP-NEXT:               DeclRefExpr {{.*}} 'gangFn'
//       DMP:           ACCLoopDirective
//  DMP-NEXT:             ACCVectorClause
//  DMP-NEXT:             ACCIndependentClause {{.*}} <implicit>
//  DMP-NEXT:             ACCSharedClause {{.*}} <implicit>
//  DMP-NEXT:               DeclRefExpr {{.*}} 'a'
//  DMP-NEXT:             impl: OMPSimdDirective
//   DMP-NOT:               OMP
//       DMP:               ForStmt
//       DMP:         ACCRoutineDeclAttr {{.*}} Implicit Gang OMPNodeKind=unknown DirectiveDiscardedForOMP
//
//   PRT-LABEL: void checkHighCallLowLoopInLambda() {
//    PRT-NEXT:   callLambdaInParallel("{{.*}}", {{.*}}, [](int *a) {
//    PRT-NEXT:     gangFn(a);
//  PRT-A-NEXT:     {{^ *}}#pragma acc loop vector{{$}}
// PRT-AO-NEXT:     {{^ *}}// #pragma omp simd{{$}}
//  PRT-O-NEXT:     {{^ *}}#pragma omp simd{{$}}
// PRT-OA-NEXT:     {{^ *}}// #pragma acc loop vector{{$}}
//    PRT-NEXT:     {{LOOP|for \(.*\) {([^{}]|[[:space:]])*}$}}
//    PRT-NEXT:   });
//    PRT-NEXT: }
//
// EXE-LABEL:checkHighCallLowLoopInLambda
//  EXE-NEXT:a[0]=50
//  EXE-NEXT:a[1]=51
//  EXE-NEXT:a[2]=52
//  EXE-NEXT:a[3]=53
//   EXE-NOT:{{.}}
void checkHighCallLowLoopInLambda() {
  callLambdaInParallel("checkHighCallLowLoopInLambda", NUM_GANGS, [](int *a) {
    gangFn(a);
    #pragma acc loop vector
    for (int i = 0; i < A_SIZE; ++i) {
      #pragma acc atomic update
      a[i] += 10;
    }
  });
}

// DMP-LABEL: FunctionDecl {{.*}} checkLowLoopHighCallInLambda 'void ()'
//       DMP:   LambdaExpr
//  DMP-NEXT:     CXXRecordDecl
//       DMP:       CXXMethodDecl
//  DMP-NEXT:         ParmVarDecl
//  DMP-NEXT:         CompoundStmt
//       DMP:           ACCLoopDirective
//  DMP-NEXT:             ACCVectorClause
//  DMP-NEXT:             ACCIndependentClause {{.*}} <implicit>
//  DMP-NEXT:             ACCSharedClause {{.*}} <implicit>
//  DMP-NEXT:               DeclRefExpr {{.*}} 'a'
//  DMP-NEXT:             impl: OMPSimdDirective
//   DMP-NOT:               OMP
//       DMP:               ForStmt
//       DMP:           CallExpr
//  DMP-NEXT:             ImplicitCastExpr
//  DMP-NEXT:               DeclRefExpr {{.*}} 'gangFn'
//       DMP:         ACCRoutineDeclAttr {{.*}} Implicit Gang OMPNodeKind=unknown DirectiveDiscardedForOMP
//
//   PRT-LABEL: void checkLowLoopHighCallInLambda() {
//    PRT-NEXT:   callLambdaInParallel("{{.*}}", {{.*}}, [](int *a) {
//  PRT-A-NEXT:     {{^ *}}#pragma acc loop vector{{$}}
// PRT-AO-NEXT:     {{^ *}}// #pragma omp simd{{$}}
//  PRT-O-NEXT:     {{^ *}}#pragma omp simd{{$}}
// PRT-OA-NEXT:     {{^ *}}// #pragma acc loop vector{{$}}
//    PRT-NEXT:     {{LOOP|for \(.*\) {([^{}]|[[:space:]])*}$}}
//    PRT-NEXT:     gangFn(a);
//    PRT-NEXT:   });
//    PRT-NEXT: }
//
// EXE-LABEL:checkLowLoopHighCallInLambda
//  EXE-NEXT:a[0]=50
//  EXE-NEXT:a[1]=51
//  EXE-NEXT:a[2]=52
//  EXE-NEXT:a[3]=53
//   EXE-NOT:{{.}}
void checkLowLoopHighCallInLambda() {
  callLambdaInParallel("checkLowLoopHighCallInLambda", NUM_GANGS, [](int *a) {
    #pragma acc loop vector
    for (int i = 0; i < A_SIZE; ++i) {
      #pragma acc atomic update
      a[i] += 10;
    }
    gangFn(a);
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
//   EXE-NOT:{{.}}
void checkGangFnCallingGangLambda() {
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
//   EXE-NOT:{{.}}
void checkGangFnCallingWorkerLambda() {
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
//   EXE-NOT:{{.}}
void checkGangFnCallingVectorLambda() {
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
//   EXE-NOT:{{.}}
void checkGangFnCallingSeqLambda() {
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
//   EXE-NOT:{{.}}
void checkWorkerFnCallingWorkerLambda() {
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
//   EXE-NOT:{{.}}
void checkVectorFnCallingVectorLambda() {
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
//   EXE-NOT:{{.}}
void checkSeqFnCallingSeqLambda() {
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
//   EXE-NOT:{{.}}
void checkLambdaCallInLambda() {
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
void checkLambdaDefAndCallInAccParallel() {
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

int main() {
  checkGangFnCallInLambda();
  checkWorkerFnCallInLambda();
  checkVectorFnCallInLambda();
  checkSeqFnCallInLambda();
  checkImpSeqFnCallInLambda();
  checkSeqFnCallInHostLambda();
  checkGangLoopInLambda();
  checkGangWorkerVectorLoopInLambda();
  checkWorkerLoopInLambda();
  checkWorkerVectorLoopInLambda();
  checkVectorLoopInLambda();
  checkSeqLoopInLambda();
  checkNakedLoopInLambda();
  checkAutoLoopInLambda();
  checkAutoGangWorkerVectorLoopInLambda();
  checkSerialLoopInLambda();
  checkHighLoopLowLoopInLambda();
  checkLowLoopHighLoopInLambda();
  checkHighCallLowCallInLambda();
  checkLowCallHighCallInLambda();
  checkHighLoopLowCallInLambda();
  checkLowCallHighLoopInLambda();
  checkHighCallLowLoopInLambda();
  checkLowLoopHighCallInLambda();
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
  return 0;
}
