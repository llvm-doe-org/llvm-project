// Check explicit "acc routine" directive on templated class member function.
//
// Clang used to report an error diagnostic claiming the routine directive
// wasn't followed by a lone function prototype or definition.

// REDEFINE: %{dmp:fc:args} = -strict-whitespace
// REDEFINE: %{exe:fc:args} = -strict-whitespace -match-full-lines \
// REDEFINE:                  -implicit-check-not='{{.}}'
// RUN: %{acc-check-dmp-cxx}
// RUN: %{acc-check-prt-cxx}
// RUN: %{acc-check-exe-cxx}

// END.

/* expected-no-diagnostics */

#include <stdio.h>

#define ARR_SIZE 4

class C {
private:
  int inc;

  // DMP-LABEL: FunctionTemplateDecl {{.*}} increment
  //  DMP-NEXT:   TemplateTypeParmDecl {{.*}} T
  //  DMP-NEXT:   CXXMethodDecl {{.*}} increment 'void (T *, T)'
  //  DMP-NEXT:     ParmVarDecl {{.*}} arr 'T *'
  //  DMP-NEXT:     ParmVarDecl {{.*}} n 'T'
  //  DMP-NEXT:     CompoundStmt
  //  DMP-NEXT:       ACCLoopDirective
  //  DMP-NEXT:         ACCGangClause
  //  DMP-NEXT:         ACCIndependentClause {{.*}} <implicit>
  //  DMP-NEXT:         ACCSharedClause {{.*}} <implicit>
  //  DMP-NEXT:           DeclRefExpr {{.*}} 'n' 'T'
  //  DMP-NEXT:           DeclRefExpr {{.*}} 'arr' 'T *'
  //  DMP-NEXT:           OMPArraySectionExpr
  //  DMP-NEXT:             CXXThisExpr {{.*}} 'C *' implicit this
  //  DMP-NEXT:             IntegerLiteral {{.*}} 'int' 0
  //  DMP-NEXT:             IntegerLiteral {{.*}} 'int' 1
  //  DMP-NEXT:             <<<NULL>>>
  //  DMP-NEXT:         impl: OMPDistributeDirective
  //       DMP:     ACCRoutineDeclAttr {{.*}} Gang OMPNodeKind=OMPDeclareTargetDecl
  //  DMP-NEXT:     OMPDeclareTargetDeclAttr {{.*}} IsOpenACCTranslation
  //   DMP-NOT:   CXXMethodDecl
  //       DMP:   CXXMethodDecl {{.*}} increment 'void (int *, int)'
  //  DMP-NEXT:     TemplateArgument type 'int'
  //  DMP-NEXT:       BuiltinType {{.*}} 'int'
  //  DMP-NEXT:     ParmVarDecl {{.*}} arr 'int *'
  //  DMP-NEXT:     ParmVarDecl {{.*}} n 'int':'int'
  //  DMP-NEXT:     CompoundStmt
  //  DMP-NEXT:       ACCLoopDirective
  //  DMP-NEXT:         ACCGangClause
  //  DMP-NEXT:         ACCIndependentClause {{.*}} <implicit>
  //  DMP-NEXT:         ACCSharedClause {{.*}} <implicit>
  //  DMP-NEXT:           DeclRefExpr {{.*}} 'n' 'int':'int'
  //  DMP-NEXT:           DeclRefExpr {{.*}} 'arr' 'int *'
  //  DMP-NEXT:           OMPArraySectionExpr
  //  DMP-NEXT:             CXXThisExpr {{.*}} 'C *' implicit this
  //  DMP-NEXT:             IntegerLiteral {{.*}} 'int' 0
  //  DMP-NEXT:             IntegerLiteral {{.*}} 'int' 1
  //  DMP-NEXT:             <<<NULL>>>
  //  DMP-NEXT:         impl: OMPDistributeDirective
  //       DMP:     ACCRoutineDeclAttr {{.*}} Gang OMPNodeKind=OMPDeclareTargetDecl
  //  DMP-NEXT:     OMPDeclareTargetDeclAttr {{.*}} IsOpenACCTranslation
  //
  //   PRT-LABEL: public:
  //  PRT-A-NEXT:   {{^ *}}#pragma acc routine gang{{$}}
  // PRT-AO-NEXT:   {{^ *}}// #pragma omp declare target{{$}}
  //  PRT-O-NEXT:   {{^ *}}#pragma omp declare target{{$}}
  // PRT-OA-NEXT:   {{^ *}}// #pragma acc routine gang{{$}}
  //    PRT-NEXT:   template <typename T> void increment(T *arr, T n) {
  //  PRT-A-NEXT:     {{^ *}}#pragma acc loop gang{{$}}
  // PRT-AO-NEXT:     {{^ *}}// #pragma omp distribute{{$}}
  //  PRT-O-NEXT:     {{^ *}}#pragma omp distribute{{$}}
  // PRT-OA-NEXT:     {{^ *}}// #pragma acc loop gang{{$}}
  //    PRT-NEXT:     for (int i = 0; i < n; ++i)
  //    PRT-NEXT:       arr[i] += {{(this->)?}}inc;
  //    PRT-NEXT:   }
  //  PRT-O-NEXT:   {{^ *}}#pragma omp end declare target{{$}}
  // FIXME: This is missing:
  // XRT-AO-NEXT:   {{^ *}}// #pragma omp end declare target{{$}}
  //
  //      EXE:arr[0]=10
  // EXE-NEXT:arr[1]=11
  // EXE-NEXT:arr[2]=12
  // EXE-NEXT:arr[3]=13
public:
  #pragma acc routine gang
  template <typename T> void increment(T *arr, T n) {
    #pragma acc loop gang
    for (int i = 0; i < n; ++i)
      arr[i] += inc;
  }
  C(int inc) : inc(inc) {}
};

int main() {
  C obj(10);
  int arr[ARR_SIZE];
  for (int i = 0; i < ARR_SIZE; ++i)
    arr[i] = i;
  #pragma acc parallel
  obj.increment(arr, ARR_SIZE);
  for (int i = 0; i < ARR_SIZE; ++i)
    printf("arr[%d]=%d\n", i, arr[i]);
  return 0;
}
