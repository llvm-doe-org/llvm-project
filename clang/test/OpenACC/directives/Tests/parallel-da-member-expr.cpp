// Check implicit and explicit data attributes for member expressions and for
// 'this' on "acc parallel", but only check cases that are not covered by
// parallel-da-member-expr.c because they are specific to C++.

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
    // Check that explicit data clause for this[0:1] behaves and that there is
    // no duplicate implicit data clause for it.
    //--------------------------------------------------------------------------

    // DMP-LABEL:"explicit DA for this\n"
    // PRT-LABEL:"explicit DA for this\n"
    // EXE-LABEL:explicit DA for this
    printf("explicit DA for this\n");
    //      DMP: ACCParallelDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 1
    // DMP-NEXT:   ACCCopyClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:     OMPArraySectionExpr
    // DMP-NEXT:       CXXThisExpr {{.*}} 'Test *' this
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 0
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:       <<<NULL>>>
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     OMPArraySectionExpr
    // DMP-NEXT:       CXXThisExpr {{.*}} 'Test *' this
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 0
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:       <<<NULL>>>
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 1
    // DMP-NEXT:     OMPMapClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       OMPArraySectionExpr
    // DMP-NEXT:         CXXThisExpr {{.*}} 'Test *' this
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 0
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:         <<<NULL>>>
    // DMP-NEXT:     CapturedStmt
    //
    //    PRT-NEXT: firstDataMember =
    //    PRT-NEXT: j =
    //    PRT-NEXT: lastDataMember =
    //  PRT-A-NEXT: #pragma acc parallel num_gangs(1) copy(this[0:1]){{$}}
    // PRT-AO-NEXT: // #pragma omp target teams num_teams(1) map(ompx_hold,tofrom: this[0:1]){{$}}
    //  PRT-O-NEXT: #pragma omp target teams num_teams(1) map(ompx_hold,tofrom: this[0:1]){{$}}
    // PRT-OA-NEXT: // #pragma acc parallel num_gangs(1) copy(this[0:1]){{$}}
    //    PRT-NEXT: {
    //         PRT: }
    //
    // EXE-NEXT:firstDataMember=71
    // EXE-NEXT:j=82
    // EXE-NEXT:lastDataMember=93
    firstDataMember = 70;
    j = 80;
    lastDataMember = 90;
    #pragma acc parallel num_gangs(1) copy(this[0:1])
    {
      Test that = *this;
      this->firstDataMember = that.firstDataMember + 1;
      j = that.j + 2;
      lastDataMember = that.lastDataMember + 3;
    }
    printf("firstDataMember=%d\n", firstDataMember);
    printf("j=%d\n", j);
    printf("lastDataMember=%d\n", lastDataMember);

    //--------------------------------------------------------------------------
    // Check that using 'this' without a member expression produces the correct
    // implicit data attribute for the entire instance.
    //--------------------------------------------------------------------------

    // DMP-LABEL:"implicit DAs for this\n"
    // PRT-LABEL:"implicit DAs for this\n"
    // EXE-LABEL:implicit DAs for this
    printf("implicit DAs for this\n");
    //      DMP: ACCParallelDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 1
    // DMP-NEXT:   ACCCopyClause {{.*}} <implicit>
    // DMP-NEXT:     OMPArraySectionExpr
    // DMP-NEXT:       CXXThisExpr {{.*}} 'Test *' this
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 0
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:       <<<NULL>>>
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     OMPArraySectionExpr
    // DMP-NEXT:       CXXThisExpr {{.*}} 'Test *' this
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 0
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:       <<<NULL>>>
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 1
    // DMP-NEXT:     OMPMapClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       OMPArraySectionExpr
    // DMP-NEXT:         CXXThisExpr {{.*}} 'Test *' this
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 0
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:         <<<NULL>>>
    // DMP-NEXT:     CapturedStmt
    //
    //    PRT-NEXT: firstDataMember =
    //    PRT-NEXT: j =
    //    PRT-NEXT: lastDataMember =
    //  PRT-A-NEXT: #pragma acc parallel num_gangs(1){{$}}
    // PRT-AO-NEXT: // #pragma omp target teams num_teams(1) map(ompx_hold,tofrom: this[0:1]){{$}}
    //  PRT-O-NEXT: #pragma omp target teams num_teams(1) map(ompx_hold,tofrom: this[0:1]){{$}}
    // PRT-OA-NEXT: // #pragma acc parallel num_gangs(1){{$}}
    //    PRT-NEXT: use(this);
    //
    // EXE-NEXT:this->firstDataMember=990
    // EXE-NEXT:this->j=880
    // EXE-NEXT:this->lastDataMember=770
    this->firstDataMember = 990;
    this->j = 880;
    this->lastDataMember = 770;
    #pragma acc parallel num_gangs(1)
    use(this);
    printf("this->firstDataMember=%d\n", this->firstDataMember);
    printf("this->j=%d\n", this->j);
    printf("this->lastDataMember=%d\n", this->lastDataMember);

    //--------------------------------------------------------------------------
    // Check explicit data clause for a data member of 'this'.
    //
    // Check explicit 'this' and implicit 'this'.  Also check when that doesn't
    // match between the data clause and the use in the associated statement.
    //--------------------------------------------------------------------------

    // DMP-LABEL:"explicit DAs for data member on explicit this\n"
    // PRT-LABEL:"explicit DAs for data member on explicit this\n"
    // EXE-LABEL:explicit DAs for data member on explicit this
    printf("explicit DAs for data member on explicit this\n");
    //      DMP: ACCParallelDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 1
    // DMP-NEXT:   ACCCopyClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:     MemberExpr {{.* ->j }}
    // DMP-NEXT:       CXXThisExpr {{.*}} 'Test *' this
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     MemberExpr {{.* ->j }}
    // DMP-NEXT:       CXXThisExpr {{.*}} 'Test *' this
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 1
    // DMP-NEXT:     OMPMapClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       MemberExpr {{.* ->j }}
    // DMP-NEXT:         CXXThisExpr {{.*}} 'Test *' this
    // DMP-NEXT:     CapturedStmt
    //
    //    PRT-NEXT: firstDataMember =
    //    PRT-NEXT: j =
    //    PRT-NEXT: lastDataMember =
    //  PRT-A-NEXT: #pragma acc parallel num_gangs(1) copy(this->j){{$}}
    // PRT-AO-NEXT: // #pragma omp target teams num_teams(1) map(ompx_hold,tofrom: this->j){{$}}
    //  PRT-O-NEXT: #pragma omp target teams num_teams(1) map(ompx_hold,tofrom: this->j){{$}}
    // PRT-OA-NEXT: // #pragma acc parallel num_gangs(1) copy(this->j){{$}}
    //    PRT-NEXT: this->j +=
    //
    // EXE-NEXT:this->firstDataMember=990
    // EXE-NEXT:this->j=884
    // EXE-NEXT:this->lastDataMember=770
    this->firstDataMember = 990;
    this->j = 880;
    this->lastDataMember = 770;
    #pragma acc parallel num_gangs(1) copy(this->j)
    this->j += 4;
    printf("this->firstDataMember=%d\n", this->firstDataMember);
    printf("this->j=%d\n", this->j);
    printf("this->lastDataMember=%d\n", this->lastDataMember);

    // DMP-LABEL:"explicit DAs for data member on explicit this (use has implicit this)\n"
    // PRT-LABEL:"explicit DAs for data member on explicit this (use has implicit this)\n"
    // EXE-LABEL:explicit DAs for data member on explicit this (use has implicit this)
    printf("explicit DAs for data member on explicit this (use has implicit this)\n");
    //      DMP: ACCParallelDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 1
    // DMP-NEXT:   ACCCopyClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:     MemberExpr {{.* ->j }}
    // DMP-NEXT:       CXXThisExpr {{.*}} 'Test *' this
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     MemberExpr {{.* ->j }}
    // DMP-NEXT:       CXXThisExpr {{.*}} 'Test *' implicit this
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 1
    // DMP-NEXT:     OMPMapClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       MemberExpr {{.* ->j }}
    // DMP-NEXT:         CXXThisExpr {{.*}} 'Test *' this
    // DMP-NEXT:     CapturedStmt
    //
    //    PRT-NEXT: firstDataMember =
    //    PRT-NEXT: j =
    //    PRT-NEXT: lastDataMember =
    //  PRT-A-NEXT: #pragma acc parallel num_gangs(1) copy(this->j){{$}}
    // PRT-AO-NEXT: // #pragma omp target teams num_teams(1) map(ompx_hold,tofrom: this->j){{$}}
    //  PRT-O-NEXT: #pragma omp target teams num_teams(1) map(ompx_hold,tofrom: this->j){{$}}
    // PRT-OA-NEXT: // #pragma acc parallel num_gangs(1) copy(this->j){{$}}
    //    PRT-NEXT: j +=
    //
    // EXE-NEXT:this->firstDataMember=990
    // EXE-NEXT:this->j=884
    // EXE-NEXT:this->lastDataMember=770
    this->firstDataMember = 990;
    this->j = 880;
    this->lastDataMember = 770;
    #pragma acc parallel num_gangs(1) copy(this->j)
    j += 4;
    printf("this->firstDataMember=%d\n", this->firstDataMember);
    printf("this->j=%d\n", this->j);
    printf("this->lastDataMember=%d\n", this->lastDataMember);

    // DMP-LABEL:"explicit DAs for data member on implicit this\n"
    // PRT-LABEL:"explicit DAs for data member on implicit this\n"
    // EXE-LABEL:explicit DAs for data member on implicit this
    printf("explicit DAs for data member on implicit this\n");
    //      DMP: ACCParallelDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 1
    // DMP-NEXT:   ACCCopyClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:     MemberExpr {{.* ->j }}
    // DMP-NEXT:       CXXThisExpr {{.*}} 'Test *' implicit this
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     MemberExpr {{.* ->j }}
    // DMP-NEXT:       CXXThisExpr {{.*}} 'Test *' implicit this
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 1
    // DMP-NEXT:     OMPMapClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       MemberExpr {{.* ->j }}
    // DMP-NEXT:         CXXThisExpr {{.*}} 'Test *' implicit this
    // DMP-NEXT:     CapturedStmt
    //
    //        PRT-NEXT: firstDataMember =
    //        PRT-NEXT: j =
    //        PRT-NEXT: lastDataMember =
    //  PRT-A-AST-NEXT: #pragma acc parallel num_gangs(1) copy(this->j){{$}}
    //  PRT-A-SRC-NEXT: #pragma acc parallel num_gangs(1) copy(j){{$}}
    //     PRT-AO-NEXT: // #pragma omp target teams num_teams(1) map(ompx_hold,tofrom: this->j){{$}}
    //      PRT-O-NEXT: #pragma omp target teams num_teams(1) map(ompx_hold,tofrom: this->j){{$}}
    // PRT-OA-AST-NEXT: // #pragma acc parallel num_gangs(1) copy(this->j){{$}}
    // PRT-OA-SRC-NEXT: // #pragma acc parallel num_gangs(1) copy(j){{$}}
    //        PRT-NEXT: j +=
    //
    // EXE-NEXT:firstDataMember=990
    // EXE-NEXT:j=884
    // EXE-NEXT:lastDataMember=770
    firstDataMember = 990;
    j = 880;
    lastDataMember = 770;
    #pragma acc parallel num_gangs(1) copy(j)
    j += 4;
    printf("firstDataMember=%d\n", firstDataMember);
    printf("j=%d\n", j);
    printf("lastDataMember=%d\n", lastDataMember);

    // DMP-LABEL:"explicit DAs for data member on implicit this (use has explicit this)\n"
    // PRT-LABEL:"explicit DAs for data member on implicit this (use has explicit this)\n"
    // EXE-LABEL:explicit DAs for data member on implicit this (use has explicit this)
    printf("explicit DAs for data member on implicit this (use has explicit this)\n");
    //      DMP: ACCParallelDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 1
    // DMP-NEXT:   ACCCopyClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:     MemberExpr {{.* ->j }}
    // DMP-NEXT:       CXXThisExpr {{.*}} 'Test *' implicit this
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     MemberExpr {{.* ->j }}
    // DMP-NEXT:       CXXThisExpr {{.*}} 'Test *' this
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 1
    // DMP-NEXT:     OMPMapClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       MemberExpr {{.* ->j }}
    // DMP-NEXT:         CXXThisExpr {{.*}} 'Test *' implicit this
    // DMP-NEXT:     CapturedStmt
    //
    //        PRT-NEXT: firstDataMember =
    //        PRT-NEXT: j =
    //        PRT-NEXT: lastDataMember =
    //  PRT-A-AST-NEXT: #pragma acc parallel num_gangs(1) copy(this->j){{$}}
    //  PRT-A-SRC-NEXT: #pragma acc parallel num_gangs(1) copy(j){{$}}
    //     PRT-AO-NEXT: // #pragma omp target teams num_teams(1) map(ompx_hold,tofrom: this->j){{$}}
    //      PRT-O-NEXT: #pragma omp target teams num_teams(1) map(ompx_hold,tofrom: this->j){{$}}
    // PRT-OA-AST-NEXT: // #pragma acc parallel num_gangs(1) copy(this->j){{$}}
    // PRT-OA-SRC-NEXT: // #pragma acc parallel num_gangs(1) copy(j){{$}}
    //        PRT-NEXT: this->j +=
    //
    // EXE-NEXT:firstDataMember=990
    // EXE-NEXT:j=884
    // EXE-NEXT:lastDataMember=770
    firstDataMember = 990;
    j = 880;
    lastDataMember = 770;
    #pragma acc parallel num_gangs(1) copy(j)
    this->j += 4;
    printf("firstDataMember=%d\n", firstDataMember);
    printf("j=%d\n", j);
    printf("lastDataMember=%d\n", lastDataMember);

    //--------------------------------------------------------------------------
    // Check that using a data member of 'this' produces the correct implicit
    // data attribute for the entire instance.
    //--------------------------------------------------------------------------

    // DMP-LABEL:"implicit DAs for data member on explicit this\n"
    // PRT-LABEL:"implicit DAs for data member on explicit this\n"
    // EXE-LABEL:implicit DAs for data member on explicit this
    printf("implicit DAs for data member on explicit this\n");
    //      DMP: ACCParallelDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 1
    // DMP-NEXT:   ACCCopyClause {{.*}} <implicit>
    // DMP-NEXT:     OMPArraySectionExpr
    // DMP-NEXT:       CXXThisExpr {{.*}} 'Test *' this
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 0
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:       <<<NULL>>>
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     OMPArraySectionExpr
    // DMP-NEXT:       CXXThisExpr {{.*}} 'Test *' this
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 0
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:       <<<NULL>>>
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 1
    // DMP-NEXT:     OMPMapClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       OMPArraySectionExpr
    // DMP-NEXT:         CXXThisExpr {{.*}} 'Test *' this
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 0
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:         <<<NULL>>>
    // DMP-NEXT:     CapturedStmt
    //
    //    PRT-NEXT: firstDataMember =
    //    PRT-NEXT: j =
    //    PRT-NEXT: lastDataMember =
    //  PRT-A-NEXT: #pragma acc parallel num_gangs(1){{$}}
    // PRT-AO-NEXT: // #pragma omp target teams num_teams(1) map(ompx_hold,tofrom: this[0:1]){{$}}
    //  PRT-O-NEXT: #pragma omp target teams num_teams(1) map(ompx_hold,tofrom: this[0:1]){{$}}
    // PRT-OA-NEXT: // #pragma acc parallel num_gangs(1){{$}}
    //    PRT-NEXT: {
    //         PRT: }
    //
    // EXE-NEXT:this->firstDataMember=995
    // EXE-NEXT:this->j=884
    // EXE-NEXT:this->lastDataMember=773
    this->firstDataMember = 990;
    this->j = 880;
    this->lastDataMember = 770;
    #pragma acc parallel num_gangs(1)
    {
      this->firstDataMember += 5;
      this->j += 4;
      this->lastDataMember += 3;
    }
    printf("this->firstDataMember=%d\n", this->firstDataMember);
    printf("this->j=%d\n", this->j);
    printf("this->lastDataMember=%d\n", this->lastDataMember);

    // DMP-LABEL:"implicit DAs for data member on implicit this\n"
    // PRT-LABEL:"implicit DAs for data member on implicit this\n"
    // EXE-LABEL:implicit DAs for data member on implicit this
    printf("implicit DAs for data member on implicit this\n");
    //      DMP: ACCParallelDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 1
    // DMP-NEXT:   ACCCopyClause {{.*}} <implicit>
    // DMP-NEXT:     OMPArraySectionExpr
    // DMP-NEXT:       CXXThisExpr {{.*}} 'Test *' implicit this
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 0
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:       <<<NULL>>>
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     OMPArraySectionExpr
    // DMP-NEXT:       CXXThisExpr {{.*}} 'Test *' implicit this
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 0
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:       <<<NULL>>>
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 1
    // DMP-NEXT:     OMPMapClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       OMPArraySectionExpr
    // DMP-NEXT:         CXXThisExpr {{.*}} 'Test *' implicit this
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 0
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:         <<<NULL>>>
    // DMP-NEXT:     CapturedStmt
    //
    //    PRT-NEXT: firstDataMember =
    //    PRT-NEXT: j =
    //    PRT-NEXT: lastDataMember =
    //  PRT-A-NEXT: #pragma acc parallel num_gangs(1){{$}}
    // PRT-AO-NEXT: // #pragma omp target teams num_teams(1) map(ompx_hold,tofrom: this[0:1]){{$}}
    //  PRT-O-NEXT: #pragma omp target teams num_teams(1) map(ompx_hold,tofrom: this[0:1]){{$}}
    // PRT-OA-NEXT: // #pragma acc parallel num_gangs(1){{$}}
    //    PRT-NEXT: {
    //         PRT: }
    //
    // EXE-NEXT:firstDataMember=995
    // EXE-NEXT:j=884
    // EXE-NEXT:lastDataMember=773
    firstDataMember = 990;
    j = 880;
    lastDataMember = 770;
    #pragma acc parallel num_gangs(1)
    {
      firstDataMember += 5;
      j += 4;
      lastDataMember += 3;
    }
    printf("firstDataMember=%d\n", firstDataMember);
    printf("j=%d\n", j);
    printf("lastDataMember=%d\n", lastDataMember);

    //--------------------------------------------------------------------------
    // Check that calling a member function of a local class instance produces
    // the correct implicit data attribute for the entire instance.
    //--------------------------------------------------------------------------

    // DMP-LABEL:"implicit DAs for member function call on local\n"
    // PRT-LABEL:"implicit DAs for member function call on local\n"
    // EXE-LABEL:implicit DAs for member function call on local
    printf("implicit DAs for member function call on local\n");
    //      DMP: ACCParallelDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 1
    // DMP-NEXT:   ACCCopyClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'local'
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'local'
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 1
    // DMP-NEXT:     OMPMapClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'local'
    //
    //         PRT: local.l = {{.}};
    //  PRT-A-NEXT: #pragma acc parallel num_gangs(1){{$}}
    // PRT-AO-NEXT: // #pragma omp target teams num_teams(1) map(ompx_hold,tofrom: local) shared(local){{$}}
    //  PRT-O-NEXT: #pragma omp target teams num_teams(1) map(ompx_hold,tofrom: local) shared(local){{$}}
    // PRT-OA-NEXT: // #pragma acc parallel num_gangs(1){{$}}
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
    #pragma acc parallel num_gangs(1)
    local.add();
    printf("local.i = %d\n", local.i);
    printf("local.j = %d\n", local.j);
    printf("local.k = %d\n", local.k);
    printf("local.l = %d\n", local.l);

    //--------------------------------------------------------------------------
    // Check that calling a member function of a local class instance produces
    // the correct implicit data attribute for the entire instance.
    //--------------------------------------------------------------------------

    // DMP-LABEL:"implicit DAs for member function call on this\n"
    // PRT-LABEL:"implicit DAs for member function call on this\n"
    // EXE-LABEL:implicit DAs for member function call on this
    printf("implicit DAs for member function call on this\n");
    //      DMP: ACCParallelDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 1
    // DMP-NEXT:   ACCCopyClause {{.*}} <implicit>
    // DMP-NEXT:     OMPArraySectionExpr
    // DMP-NEXT:       CXXThisExpr {{.*}} 'Test *' implicit this
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 0
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:       <<<NULL>>>
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     OMPArraySectionExpr
    // DMP-NEXT:       CXXThisExpr {{.*}} 'Test *' implicit this
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 0
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:       <<<NULL>>>
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 1
    // DMP-NEXT:     OMPMapClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       OMPArraySectionExpr
    // DMP-NEXT:         CXXThisExpr {{.*}} 'Test *' implicit this
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 0
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:         <<<NULL>>>
    //
    //         PRT: l = {{.}};
    //  PRT-A-NEXT: #pragma acc parallel num_gangs(1){{$}}
    // PRT-AO-NEXT: // #pragma omp target teams num_teams(1) map(ompx_hold,tofrom: this[0:1]){{$}}
    //  PRT-O-NEXT: #pragma omp target teams num_teams(1) map(ompx_hold,tofrom: this[0:1]){{$}}
    // PRT-OA-NEXT: // #pragma acc parallel num_gangs(1){{$}}
    //
    // EXE-NEXT:i = 15
    // EXE-NEXT:j = 26
    // EXE-NEXT:k = 37
    // EXE-NEXT:l = 48
    i = 5;
    j = 6;
    k = 7;
    l = 8;
    #pragma acc parallel num_gangs(1)
    add();
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
