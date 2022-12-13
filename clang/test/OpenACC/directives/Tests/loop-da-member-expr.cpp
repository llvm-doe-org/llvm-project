// Check implicit and explicit data attributes for member expressions and for
// 'this' on "acc loop", but only check cases that are not covered by
// loop-da-member-expr.c because they are specific to C++.

// REDEFINE: %{dmp:fc:args} = -strict-whitespace
// REDEFINE: %{exe:fc:args} = -strict-whitespace -match-full-lines \
// REDEFINE:                  -implicit-check-not='{{.}}'
// RUN: %{acc-check-dmp-cxx}
// RUN: %{acc-check-prt-cxx}
// RUN: %{acc-check-exe-cxx}

// END.

/* expected-no-diagnostics */

#include <stdio.h>

// A way to use a 'this' pointer without a member expression.  Don't make it a
// member of Test, or any call to it will be a member expression.
void use(void *p) {}

class Test {
public:
  int firstDataMember;
  int i;
  int j;
  int k;
  int l;
  int lastDataMember;
  void add() { i += 10; j += 20; k += 30; l += 40; }
  void run() {
    //--------------------------------------------------------------------------
    // Check that using 'this' without a member expression produces the correct
    // implicit data attribute for the entire instance.
    //--------------------------------------------------------------------------

    // DMP-LABEL:"implicit DAs for this\n"
    // PRT-LABEL:"implicit DAs for this\n"
    // EXE-LABEL:implicit DAs for this
    printf("implicit DAs for this\n");
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCGangClause
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     OMPArraySectionExpr
    // DMP-NEXT:       CXXThisExpr {{.*}} 'Test *' this
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 0
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:       <<<NULL>>>
    // DMP-NEXT:   impl: OMPDistributeDirective
    //  DMP-NOT:     OMP
    //      DMP:     ForStmt
    //
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCSeqClause
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'j'
    // DMP-NEXT:     OMPArraySectionExpr
    // DMP-NEXT:       CXXThisExpr {{.*}} 'Test *' this
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 0
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:       <<<NULL>>>
    // DMP-NEXT:   impl: ForStmt
    //
    //         PRT: l =
    //  PRT-A-NEXT: #pragma acc loop gang{{$}}
    // PRT-AO-NEXT: // #pragma omp distribute{{$}}
    //  PRT-O-NEXT: #pragma omp distribute{{$}}
    // PRT-OA-NEXT: // #pragma acc loop gang{{$}}
    //    PRT-NEXT: for ({{.*}})
    //    PRT-NEXT:   use(this);
    //
    //    PRT-NEXT: int j;
    //  PRT-A-NEXT: #pragma acc loop seq
    // PRT-AO-SAME: // discarded in OpenMP translation
    //  PRT-A-SAME: {{$}}
    // PRT-OA-NEXT: // #pragma acc loop seq // discarded in OpenMP translation{{$}}
    //    PRT-NEXT: for ({{.*}})
    //    PRT-NEXT:   use(this);
    //
    // EXE-NEXT:this->i=990
    // EXE-NEXT:this->j=880
    // EXE-NEXT:this->k=770
    // EXE-NEXT:this->l=660
    #pragma acc parallel num_gangs(1) copyout(this[0:1])
    {
      this->i = 990;
      this->j = 880;
      this->k = 770;
      this->l = 660;
      #pragma acc loop gang
      for (int i = 0; i < 4; ++i)
        use(this);
      int j;
      #pragma acc loop seq
      for (j = 0; j < 4; ++j)
        use(this);
    }
    printf("this->i=%d\n", this->i);
    printf("this->j=%d\n", this->j);
    printf("this->k=%d\n", this->k);
    printf("this->l=%d\n", this->l);

    //--------------------------------------------------------------------------
    // Check explicit data clause for a data member of 'this'.
    //
    // Check explicit 'this' and implicit 'this'.  Also check when that doesn't
    // match between the data clause and the use in the associated statement.
    //--------------------------------------------------------------------------

    // DMP-LABEL:"explicit DAs for data member on this\n"
    // PRT-LABEL:"explicit DAs for data member on this\n"
    // EXE-LABEL:explicit DAs for data member on this
    printf("explicit DAs for data member on this\n");
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCGangClause
    // DMP-NEXT:   ACCWorkerClause
    // DMP-NEXT:   ACCVectorClause
    // DMP-NEXT:   ACCReductionClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:     MemberExpr {{.* ->i }}
    // DMP-NEXT:       CXXThisExpr {{.*}} 'Test *' this
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPDistributeParallelForSimdDirective
    // DMP-NEXT:     OMPReductionClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} OMPCapturedExpr {{.*}} 'i'
    //  DMP-NOT:     OMP
    //      DMP:     ForStmt
    //
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCGangClause
    // DMP-NEXT:   ACCWorkerClause
    // DMP-NEXT:   ACCVectorClause
    // DMP-NEXT:   ACCReductionClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:     MemberExpr {{.* ->j }}
    // DMP-NEXT:       CXXThisExpr {{.*}} 'Test *' this
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPDistributeParallelForSimdDirective
    // DMP-NEXT:     OMPReductionClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} OMPCapturedExpr {{.*}} 'j'
    //  DMP-NOT:     OMP
    //      DMP:     ForStmt
    //
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCGangClause
    // DMP-NEXT:   ACCWorkerClause
    // DMP-NEXT:   ACCVectorClause
    // DMP-NEXT:   ACCReductionClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:     MemberExpr {{.* ->k }}
    // DMP-NEXT:       CXXThisExpr {{.*}} 'Test *' implicit this
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPDistributeParallelForSimdDirective
    // DMP-NEXT:     OMPReductionClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} OMPCapturedExpr {{.*}} 'k'
    //  DMP-NOT:     OMP
    //      DMP:     ForStmt
    //
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCGangClause
    // DMP-NEXT:   ACCWorkerClause
    // DMP-NEXT:   ACCVectorClause
    // DMP-NEXT:   ACCReductionClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:     MemberExpr {{.* ->l }}
    // DMP-NEXT:       CXXThisExpr {{.*}} 'Test *' implicit this
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPDistributeParallelForSimdDirective
    // DMP-NEXT:     OMPReductionClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} OMPCapturedExpr {{.*}} 'l'
    //  DMP-NOT:     OMP
    //      DMP:     ForStmt
    //
    //             PRT: this->l =
    //             PRT: {
    //      PRT-A-NEXT:   #pragma acc loop gang worker vector reduction(+: this->i){{$}}
    //     PRT-AO-NEXT:   // #pragma omp distribute parallel for simd reduction(+: this->i){{$}}
    //      PRT-O-NEXT:   #pragma omp distribute parallel for simd reduction(+: this->i){{$}}
    //     PRT-OA-NEXT:   // #pragma acc loop gang worker vector reduction(+: this->i){{$}}
    //        PRT-NEXT:   for ({{.*}})
    //        PRT-NEXT:     this->i +=
    //      PRT-A-NEXT:   #pragma acc loop gang worker vector reduction(+: this->j){{$}}
    //     PRT-AO-NEXT:   // #pragma omp distribute parallel for simd reduction(+: this->j){{$}}
    //      PRT-O-NEXT:   #pragma omp distribute parallel for simd reduction(+: this->j){{$}}
    //     PRT-OA-NEXT:   // #pragma acc loop gang worker vector reduction(+: this->j){{$}}
    //        PRT-NEXT:   for ({{.*}})
    //        PRT-NEXT:     j +=
    //  PRT-A-AST-NEXT:   #pragma acc loop gang worker vector reduction(+: this->k){{$}}
    //  PRT-A-SRC-NEXT:   #pragma acc loop gang worker vector reduction(+: k){{$}}
    //     PRT-AO-NEXT:   // #pragma omp distribute parallel for simd reduction(+: this->k){{$}}
    //      PRT-O-NEXT:   #pragma omp distribute parallel for simd reduction(+: this->k){{$}}
    // PRT-OA-AST-NEXT:   // #pragma acc loop gang worker vector reduction(+: this->k){{$}}
    // PRT-OA-SRC-NEXT:   // #pragma acc loop gang worker vector reduction(+: k){{$}}
    //        PRT-NEXT:   for ({{.*}})
    //        PRT-NEXT:     this->k +=
    //  PRT-A-AST-NEXT:   #pragma acc loop gang worker vector reduction(+: this->l){{$}}
    //  PRT-A-SRC-NEXT:   #pragma acc loop gang worker vector reduction(+: l){{$}}
    //     PRT-AO-NEXT:   // #pragma omp distribute parallel for simd reduction(+: this->l){{$}}
    //      PRT-O-NEXT:   #pragma omp distribute parallel for simd reduction(+: this->l){{$}}
    // PRT-OA-AST-NEXT:   // #pragma acc loop gang worker vector reduction(+: this->l){{$}}
    // PRT-OA-SRC-NEXT:   // #pragma acc loop gang worker vector reduction(+: l){{$}}
    //        PRT-NEXT:   for ({{.*}})
    //        PRT-NEXT:     l +=
    //        PRT-NEXT: }
    //
    // EXE-NEXT:this->i=994
    // EXE-NEXT:this->j=884
    // EXE-NEXT:this->k=778
    // EXE-NEXT:this->l=668
    this->i = 990;
    this->j = 880;
    this->k = 770;
    this->l = 660;
    #pragma acc parallel num_gangs(4) \
                         reduction(+:this->i, this->j, this->k, this->l)
    {
      #pragma acc loop gang worker vector reduction(+: this->i)
      for (int x = 0; x < 4; ++x)
        this->i += 1;
      #pragma acc loop gang worker vector reduction(+: this->j)
      for (int x = 0; x < 4; ++x)
        j += 1;
      #pragma acc loop gang worker vector reduction(+: k)
      for (int x = 0; x < 4; ++x)
        this->k += 2;
      #pragma acc loop gang worker vector reduction(+: l)
      for (int x = 0; x < 4; ++x)
        l += 2;
    }
    printf("this->i=%d\n", this->i);
    printf("this->j=%d\n", this->j);
    printf("this->k=%d\n", this->k);
    printf("this->l=%d\n", this->l);

    //--------------------------------------------------------------------------
    // Check that using a data member of 'this' produces the correct implicit
    // data attribute for the entire instance.
    //--------------------------------------------------------------------------

    // DMP-LABEL:"implicit DAs for data member on explicit this\n"
    // PRT-LABEL:"implicit DAs for data member on explicit this\n"
    // EXE-LABEL:implicit DAs for data member on explicit this
    printf("implicit DAs for data member on explicit this\n");
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCGangClause
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     OMPArraySectionExpr
    // DMP-NEXT:       CXXThisExpr {{.*}} 'Test *' this
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 0
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:       <<<NULL>>>
    // DMP-NEXT:   impl: OMPDistributeDirective
    //  DMP-NOT:     OMP
    //      DMP:     ForStmt
    //
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCSeqClause
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'j'
    // DMP-NEXT:     OMPArraySectionExpr
    // DMP-NEXT:       CXXThisExpr {{.*}} 'Test *' this
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 0
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:       <<<NULL>>>
    // DMP-NEXT:   impl: ForStmt
    //
    //         PRT: l =
    //  PRT-A-NEXT: #pragma acc loop gang{{$}}
    // PRT-AO-NEXT: // #pragma omp distribute{{$}}
    //  PRT-O-NEXT: #pragma omp distribute{{$}}
    // PRT-OA-NEXT: // #pragma acc loop gang{{$}}
    //    PRT-NEXT: for ({{.*}})
    //    PRT-NEXT:   ;
    //
    //    PRT-NEXT: int j;
    //  PRT-A-NEXT: #pragma acc loop seq
    // PRT-AO-SAME: // discarded in OpenMP translation
    //  PRT-A-SAME: {{$}}
    // PRT-OA-NEXT: // #pragma acc loop seq // discarded in OpenMP translation{{$}}
    //    PRT-NEXT: for ({{.*}})
    //    PRT-NEXT:   ;
    //
    // EXE-NEXT:this->i=990
    // EXE-NEXT:this->j=880
    // EXE-NEXT:this->k=774
    // EXE-NEXT:this->l=668
    #pragma acc parallel num_gangs(1) copyout(this[0:1])
    {
      this->i = 990;
      this->j = 880;
      this->k = 770;
      this->l = 660;
      #pragma acc loop gang
      for (int i = 0; i < 4; ++i)
        this->k += 1;
      int j;
      #pragma acc loop seq
      for (j = 0; j < 4; ++j)
        this->l += 2;
    }
    printf("this->i=%d\n", this->i);
    printf("this->j=%d\n", this->j);
    printf("this->k=%d\n", this->k);
    printf("this->l=%d\n", this->l);

    // DMP-LABEL:"implicit DAs for data member on implicit this\n"
    // PRT-LABEL:"implicit DAs for data member on implicit this\n"
    // EXE-LABEL:implicit DAs for data member on implicit this
    printf("implicit DAs for data member on implicit this\n");
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCGangClause
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     OMPArraySectionExpr
    // DMP-NEXT:       CXXThisExpr {{.*}} 'Test *' implicit this
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 0
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:       <<<NULL>>>
    // DMP-NEXT:   impl: OMPDistributeDirective
    //  DMP-NOT:     OMP
    //      DMP:     ForStmt
    //
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCSeqClause
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'j'
    // DMP-NEXT:     OMPArraySectionExpr
    // DMP-NEXT:       CXXThisExpr {{.*}} 'Test *' implicit this
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 0
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:       <<<NULL>>>
    // DMP-NEXT:   impl: ForStmt
    //
    //         PRT: l =
    //  PRT-A-NEXT: #pragma acc loop gang{{$}}
    // PRT-AO-NEXT: // #pragma omp distribute{{$}}
    //  PRT-O-NEXT: #pragma omp distribute{{$}}
    // PRT-OA-NEXT: // #pragma acc loop gang{{$}}
    //    PRT-NEXT: for ({{.*}})
    //    PRT-NEXT:   ;
    //
    //    PRT-NEXT: int j;
    //  PRT-A-NEXT: #pragma acc loop seq
    // PRT-AO-SAME: // discarded in OpenMP translation
    //  PRT-A-SAME: {{$}}
    // PRT-OA-NEXT: // #pragma acc loop seq // discarded in OpenMP translation{{$}}
    //    PRT-NEXT: for ({{.*}})
    //    PRT-NEXT:   ;
    //
    // EXE-NEXT:i=990
    // EXE-NEXT:j=880
    // EXE-NEXT:k=774
    // EXE-NEXT:l=668
    #pragma acc parallel num_gangs(1) copyout(this[0:1])
    {
      i = 990;
      j = 880;
      k = 770;
      l = 660;
      #pragma acc loop gang
      for (int i = 0; i < 4; ++i)
        k += 1;
      int j;
      #pragma acc loop seq
      for (j = 0; j < 4; ++j)
        l += 2;
    }
    printf("i=%d\n", i);
    printf("j=%d\n", j);
    printf("k=%d\n", k);
    printf("l=%d\n", l);

    //--------------------------------------------------------------------------
    // Check that calling a member function of a local class instance produces
    // the correct implicit data attribute for the entire instance.
    //--------------------------------------------------------------------------
  
    // DMP-LABEL:"implicit DAs for member function call on local\n"
    // PRT-LABEL:"implicit DAs for member function call on local\n"
    // EXE-LABEL:implicit DAs for member function call on local
    printf("implicit DAs for member function call on local\n");
  
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCIndependentClause
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'local'
    // DMP-NEXT:   ACCGangClause
    //      DMP:   impl: OMPDistributeDirective
    //  DMP-NOT:     OMP
    //      DMP:     ForStmt
    //
    //         PRT: local.l = {{.}};
    //         PRT: {
    //  PRT-A-NEXT:   #pragma acc loop{{$}}
    // PRT-AO-NEXT:   // #pragma omp distribute{{$}}
    // PRT-OA-NEXT:   #pragma omp distribute{{$}}
    // PRT-OA-NEXT:   // #pragma acc loop{{$}}
    //         PRT: }
    //
    // EXE-NEXT:local.i = 15
    // EXE-NEXT:local.j = 26
    // EXE-NEXT:local.k = 37
    // EXE-NEXT:local.l = 48
    Test local;
    local.i = 5;
    local.j = 6;
    local.k = 7;
    local.l = 8;
    #pragma acc parallel copy(local)
    {
      #pragma acc loop
      for (int i = 0; i < 1; ++i)
        local.add();
    }
    printf("local.i = %d\n", local.i);
    printf("local.j = %d\n", local.j);
    printf("local.k = %d\n", local.k);
    printf("local.l = %d\n", local.l);

    //--------------------------------------------------------------------------
    // Check that calling a member function of 'this' produces the correct
    // implicit data attribute for the entire instance.
    //--------------------------------------------------------------------------

    // DMP-LABEL:"implicit DAs for member function call on this\n"
    // PRT-LABEL:"implicit DAs for member function call on this\n"
    // EXE-LABEL:implicit DAs for member function call on this
    printf("implicit DAs for member function call on this\n");
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCIndependentClause
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     OMPArraySectionExpr
    // DMP-NEXT:       CXXThisExpr {{.*}} 'Test *' implicit this 
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 0 
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1 
    // DMP-NEXT:       <<<NULL>>> 
    // DMP-NEXT:   ACCGangClause
    //      DMP:   impl: OMPDistributeDirective
    //  DMP-NOT:     OMP
    //      DMP:     ForStmt
    //
    //         PRT: l = {{.}};
    //         PRT: {
    //  PRT-A-NEXT:   #pragma acc loop{{$}}
    // PRT-AO-NEXT:   // #pragma omp distribute{{$}}
    // PRT-OA-NEXT:   #pragma omp distribute{{$}}
    // PRT-OA-NEXT:   // #pragma acc loop{{$}}
    //         PRT: }
    //
    // EXE-NEXT:i = 12
    // EXE-NEXT:j = 23
    // EXE-NEXT:k = 34
    // EXE-NEXT:l = 45
    i = 2;
    j = 3;
    k = 4;
    l = 5;
    #pragma acc parallel copy(this[0:1])
    {
      #pragma acc loop
      for (int i = 0; i < 1; ++i)
        add();
    }
    printf("i = %d\n", i);
    printf("j = %d\n", j);
    printf("k = %d\n", k);
    printf("l = %d\n", l);
  }
};

int main() {
  Test test;
  test.run();
  return 0;
}
