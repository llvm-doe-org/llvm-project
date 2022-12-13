// Check reduction clause on "acc loop" and on "acc parallel loop".
//
// For aliased clauses, we arbitrarily cycle through the aliases throughout
// this test to give confidence they all are handled equivalently in sema and
// codegen.  We do not attempt to check every alias in every scenario as that
// would make the test much slower and more difficult to maintain.

// RUN: %{acc-check-dmp}
// RUN: %{acc-check-prt}
// RUN: %{acc-check-exe}

// END.

/* expected-no-diagnostics */

// FIXME: For amdgcn, we get the following compile error:
//
//   File /home/jdenny/llvm/build/lib/clang/14.0.0/include/__clang_hip_math.h Line 23: 'omp.h' file not found
#if !TGT_AMDGCN
# include <complex.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#if TGT_USE_STDIO
# define TGT_PRINTF(...) printf(__VA_ARGS__)
#else
# define TGT_PRINTF(...)
#endif

#pragma acc routine gang
static void withinGangFn(int param);
#pragma acc routine worker
static void withinWorkerFn(int param);
#pragma acc routine vector
static void withinVectorFn(int param);
#pragma acc routine seq
static void withinSeqFn(int param);

//==============================================================================
// Check within parallel constructs.
//==============================================================================

int main() {
  //----------------------------------------------------------------------------
  // Explicit seq.
  //----------------------------------------------------------------------------

  // DMP-LABEL: StringLiteral {{.*}} "explicit seq\n"
  // PRT-LABEL: printf("explicit seq\n");
  //   EXE-NOT: {{.}}
  // EXE-LABEL: explicit seq{{$}}
  //   EXE-NOT: {{.}}
  printf("explicit seq\n");

  // Reduction var is private at loop, explicitly or implicitly.

  // PRT-NEXT: {
  {
    // PRT-NEXT: int out0 = 10;
    // PRT-NEXT: int out1 = 30;
    // PRT-NEXT: int out2 = 40;
    int out0 = 10;
    int out1 = 30;
    int out2 = 40;
    // DMP:      ACCParallelDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCReductionClause {{.*}} '*'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out0' 'int'
    // DMP-NEXT:   ACCFirstprivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out1' 'int'
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out2' 'int'
    // DMP-NEXT:   ACCNomapClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out1' 'int'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out2' 'int'
    // DMP-NEXT:   ACCCopyClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out0' 'int'
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out0' 'int'
    // DMP-NEXT:     OMPFirstprivateClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out1' 'int'
    // DMP-NEXT:     OMPPrivateClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out2' 'int'
    // DMP-NEXT:     OMPMapClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out0' 'int'
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(2) reduction(*: out0) firstprivate(out1) private(out2){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) reduction(*: out0) firstprivate(out1) private(out2) map(ompx_hold,tofrom: out0){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) reduction(*: out0) firstprivate(out1) private(out2) map(ompx_hold,tofrom: out0){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2) reduction(*: out0) firstprivate(out1) private(out2){{$}}
    #pragma acc parallel num_gangs(2) reduction(*: out0) firstprivate(out1) private(out2)
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      // PRT-NEXT: float in = 10;
      float in = 10;
      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCSeqClause
      // DMP-NEXT:   ACCReductionClause {{.*}} '*'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out0' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out1' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out2' 'int'
      // DMP-NEXT:   ACCReductionClause {{.*}} '+'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'in' 'float'
      // DMP-NEXT:   impl: ForStmt
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop seq reduction(*: out0,out1,out2) reduction(+: in)
      // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
      // PRT-A-SAME:  {{^$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop seq reduction(*: out0,out1,out2) reduction(+: in) // discarded in OpenMP translation{{$}}
      #pragma acc loop seq reduction(*: out0,out1,out2) reduction(+: in)
      // PRT-NEXT: for ({{.*}}) {
      for (int i = 0; i < 2; ++i) {
        // DMP: CompoundAssignOperator {{.*}} 'int' '*='
        // PRT-NEXT: out0 *= 2;
        out0 *= 2;
        // DMP: CompoundAssignOperator {{.*}} 'int' '*='
        // PRT-NEXT: out1 *= 2;
        out1 *= 2;
        // DMP: CompoundAssignOperator {{.*}} 'int' '*='
        // PRT-NEXT: out2 *= 2;
        out2 *= 2;
        // DMP: CompoundAssignOperator {{.*}} 'float' '+='
        // PRT-NEXT: in += 2;
        in += 2;
      } // PRT-NEXT: }
      // DMP: CallExpr
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // EXE-TGT-USE-STDIO-DAG: out1 = 120
      // EXE-TGT-USE-STDIO-DAG: out1 = 120
      TGT_PRINTF("out1 = %d\n", out1);
      // DMP: CallExpr
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // EXE-TGT-USE-STDIO-DAG: in = 14.0
      // EXE-TGT-USE-STDIO-DAG: in = 14.0
      TGT_PRINTF("in = %.1f\n", in);
    } // PRT-NEXT: }
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: out0 = 160
    printf("out0 = %d\n", out0);
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: out1 = 30
    printf("out1 = %d\n", out1);
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: out2 = 40
    printf("out2 = %d\n", out2);
  } // PRT-NEXT: }

  // Reduction var is not private at loop due to explicit copy/copyin/copyout
  // clause.

  // PRT-NEXT: {
  {
    // PRT-NEXT: double out0 =
    // PRT-NEXT: double out1 =
    // PRT-NEXT: double out2 =
    double out0 = 0.3;
    double out1 = 1.4;
    double out2 = 2.5;
    // DMP:      ACCParallelDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCCopyClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out0' 'double'
    // DMP-NEXT:   ACCCopyinClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out1' 'double'
    // DMP-NEXT:   ACCCopyoutClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out2' 'double'
    // DMP-NEXT:   ACCReductionClause {{.*}} <implicit> '+'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out0' 'double'
    // DMP-NEXT:   ACCReductionClause {{.*}} <implicit> '+'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out1' 'double'
    // DMP-NEXT:   ACCReductionClause {{.*}} <implicit> '+'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out2' 'double'
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     OMPMapClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out0' 'double'
    // DMP-NEXT:     OMPMapClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out1' 'double'
    // DMP-NEXT:     OMPMapClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out2' 'double'
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out0' 'double'
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out1' 'double'
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out2' 'double'
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(2) copy(out0) copyin(out1) copyout(out2){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) map(ompx_hold,tofrom: out0) map(ompx_hold,to: out1) map(ompx_hold,from: out2) reduction(+: out0) reduction(+: out1) reduction(+: out2){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) map(ompx_hold,tofrom: out0) map(ompx_hold,to: out1) map(ompx_hold,from: out2) reduction(+: out0) reduction(+: out1) reduction(+: out2){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2) copy(out0) copyin(out1) copyout(out2){{$}}
    #pragma acc parallel num_gangs(2) copy(out0) copyin(out1) copyout(out2)
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      // Make sure accessing the variables before the reduction doesn't
      // produce a conflicting implicit firstprivate clause.
      // DMP: CallExpr
      // DMP: CallExpr
      // DMP: CallExpr
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // EXE-TGT-USE-STDIO-DAG: out0 init = 0.0{{$}}
      // EXE-TGT-USE-STDIO-DAG: out0 init = 0.0{{$}}
      // EXE-TGT-USE-STDIO-DAG: out1 init = 0.0{{$}}
      // EXE-TGT-USE-STDIO-DAG: out1 init = 0.0{{$}}
      // EXE-TGT-USE-STDIO-DAG: out2 init = 0.0{{$}}
      // EXE-TGT-USE-STDIO-DAG: out2 init = 0.0{{$}}
      TGT_PRINTF("out0 init = %.1f\n", out0);
      TGT_PRINTF("out1 init = %.1f\n", out1);
      TGT_PRINTF("out2 init = %.1f\n", out2);
      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCSeqClause
      // DMP-NEXT:   ACCReductionClause {{.*}} '+'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out0' 'double'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out1' 'double'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out2' 'double'
      // DMP-NEXT:   impl: ForStmt
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop seq reduction(+: out0,out1,out2)
      // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
      // PRT-A-SAME:  {{^$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop seq reduction(+: out0,out1,out2) // discarded in OpenMP translation{{$}}
      #pragma acc loop seq reduction(+: out0,out1,out2)
      // PRT-NEXT: for ({{.*}}) {
      for (int i = 0; i < 4; ++i) {
        // DMP: CompoundAssignOperator {{.*}} '+='
        // DMP: CompoundAssignOperator {{.*}} '+='
        // DMP: CompoundAssignOperator {{.*}} '+='
        // PRT-NEXT: out0 +=
        // PRT-NEXT: out1 +=
        // PRT-NEXT: out2 +=
        out0 += -1.1;
        out1 += -1.1;
        out2 += -1.1;
      } // PRT-NEXT: }
    } // PRT-NEXT: }
    // DMP: CallExpr
    // PRT-NEXT: printf
    // PRT-NEXT: printf
    // PRT-NEXT: printf
    // EXE-NEXT: out0 = -8.5
    // EXE-HOST-NEXT: out1 = -7.4
    // EXE-HOST-NEXT: out2 = -6.3
    // EXE-TGT-X86_64-NEXT: out1 = 1.4
    // EXE-TGT-X86_64-NEXT: out2 =
    // EXE-TGT-PPC64LE-NEXT: out1 = 1.4
    // EXE-TGT-PPC64LE-NEXT: out2 =
    // EXE-TGT-NVPTX64-NEXT: out1 = 1.4
    // EXE-TGT-NVPTX64-NEXT: out2 =
    // EXE-TGT-AMDGCN-NEXT: out1 = 1.4
    // EXE-TGT-AMDGCN-NEXT: out2 =
    printf("out0 = %.1f\n", out0);
    printf("out1 = %.1f\n", out1);
    printf("out2 = %.1f\n", out2);
  } // PRT-NEXT: }

  // Reduction var is not private at loop due to copy clause implied by
  // reduction on combined construct.

  // PRT-NEXT: {
  {
    // PRT-NEXT: int out = 10;
    int out = 10;
    // DMP:      ACCParallelLoopDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCSeqClause
    // DMP-NEXT:   ACCReductionClause {{.*}} '*'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:   effect: ACCParallelDirective
    // DMP-NEXT:     ACCNumGangsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     ACCCopyClause {{.*}} <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:     ACCReductionClause {{.*}} <implicit> '*'
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:     impl: OMPTargetTeamsDirective
    // DMP-NEXT:       OMPNum_teamsClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:       OMPMapClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:       OMPReductionClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out' 'int'
    // DMP:          ACCLoopDirective
    // DMP-NEXT:       ACCSeqClause
    // DMP-NEXT:       ACCReductionClause {{.*}} '*'
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:       impl: ForStmt
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel loop num_gangs(2) seq reduction(*: out){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) map(ompx_hold,tofrom: out) reduction(*: out){{$}}
    //
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) map(ompx_hold,tofrom: out) reduction(*: out){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) seq reduction(*: out){{$}}
    //
    // PRT-NEXT: for ({{.*}}) {
    #pragma acc parallel loop num_gangs(2) seq reduction(*: out)
    for (int i = 0; i < 2; ++i) {
      // DMP: CompoundAssignOperator {{.*}} 'int' '*='
      // PRT-NEXT: out *= 2;
      out *= 2;
    } // PRT-NEXT: }
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: out = 160
    printf("out = %d\n", out);
  } // PRT-NEXT: }

  //----------------------------------------------------------------------------
  // Implicit gang.
  //----------------------------------------------------------------------------

  // DMP-LABEL: StringLiteral {{.*}} "implicit gang\n"
  // PRT-LABEL: printf("implicit gang\n");
  //   EXE-NOT: {{.}}
  // EXE-LABEL: implicit gang{{$}}
  //   EXE-NOT: {{.}}
  printf("implicit gang\n");

  // acc parallel has reduction already.

  // PRT-NEXT: {
  {
    // PRT-NEXT: enum E {{{[[:space:]]*}}
    // PRT-SAME:   E0,{{[[:space:]]*}}
    // PRT-SAME:   E1{{[[:space:]]*}}
    // PRT-SAME: } out = E0;
    enum E {E0, E1} out = E0;
    // DMP:      ACCParallelDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCReductionClause {{.*}} 'max'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'enum E'
    // DMP-NEXT:   ACCCopyClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'enum E'
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'enum E'
    // DMP-NEXT:     OMPMapClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'enum E'
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(2) reduction(max: out){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) reduction(max: out) map(ompx_hold,tofrom: out){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) reduction(max: out) map(ompx_hold,tofrom: out){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2) reduction(max: out){{$}}
    #pragma acc parallel num_gangs(2) reduction(max: out)
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      // PRT-NEXT: int arr[] = {10000, 1};
      // PRT-NEXT: int *in = arr;
      int arr[] = {10000, 1};
      int *in = arr;
      //      DMP: ACCLoopDirective
      // DMP-NEXT:   ACCReductionClause {{.*}} 'max'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'enum E'
      // DMP-NEXT:   ACCReductionClause {{.*}} 'min'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'in' 'int *'
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'arr' 'int[2]'
      // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
      // DMP-NEXT:   impl: OMPDistributeDirective
      //  DMP-NOT:     OMP
      //      DMP:     ForStmt
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop reduction(max: out) reduction(min: in){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
      // PRT-O-NEXT:  {{^ *}}#pragma omp distribute{{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop reduction(max: out) reduction(min: in){{$}}
      #pragma acc loop reduction(max: out) reduction(min: in)
      // PRT-NEXT: for ({{.*}}) {
      for (int i = 0; i < 2; ++i) {
        // DMP: ConditionalOperator
        // PRT-NEXT: out = E1 > out ? E1 : out;
        out = E1 > out ? E1 : out;
        // DMP: ConditionalOperator
        // PRT-NEXT: in = arr + i < in ? arr + i : in;
        in = arr + i < in ? arr + i : in;
      } // PRT-NEXT: }
      // DMP: CallExpr
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // EXE-TGT-USE-STDIO-NEXT: in == arr: 1
      // EXE-TGT-USE-STDIO-NEXT: in == arr: 1
      TGT_PRINTF("in == arr: %d\n", in == arr);
    } // PRT-NEXT: }
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: out = 1
    printf("out = %d\n", out);
  } // PRT-NEXT: }

  // Only acc loop has gang reduction.

  // PRT-NEXT: {
  {
    // PRT-NEXT: enum E {{{[[:space:]]*}}
    // PRT-SAME:   E0,{{[[:space:]]*}}
    // PRT-SAME:   E1{{[[:space:]]*}}
    // PRT-SAME: } out = E0;
    enum E {E0, E1} out = E0;
    // DMP:      ACCParallelDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCCopyClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'enum E'
    // DMP-NEXT:   ACCReductionClause {{.*}} <implicit> 'max'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'enum E'
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     OMPMapClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'enum E'
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'enum E'
    // DMP:        ACCLoopDirective
    // DMP-NEXT:     ACCReductionClause {{.*}} 'max'
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'enum E'
    // DMP-NEXT:     ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:     ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:     impl: OMPDistributeDirective
    // DMP:            ForStmt
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(2) copy(out){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) map(ompx_hold,tofrom: out) reduction(max: out){{$}}
    // PRT-A-NEXT:  {{^ *}}#pragma acc loop reduction(max: out){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
    //
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) map(ompx_hold,tofrom: out) reduction(max: out){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2) copy(out){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp distribute{{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop reduction(max: out){{$}}
    //
    // PRT-NEXT: for ({{.*}}) {
    #pragma acc parallel num_gangs(2) copy(out)
    #pragma acc loop reduction(max: out)
    for (int i = 0; i < 2; ++i) {
      // DMP: ConditionalOperator
      // PRT-NEXT: out = E1 > out ? E1 : out;
      out = E1 > out ? E1 : out;
    } // PRT-NEXT: }
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: out = 1
    printf("out = %d\n", out);
  } // PRT-NEXT: }

  // acc parallel loop with gang reduction.

  // PRT-NEXT: {
  {
    // PRT-NEXT: enum E {{{[[:space:]]*}}
    // PRT-SAME:   E0,{{[[:space:]]*}}
    // PRT-SAME:   E1{{[[:space:]]*}}
    // PRT-SAME: } out = E0;
    enum E {E0, E1} out = E0;
    // DMP:      ACCParallelLoopDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCReductionClause {{.*}} 'max'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'enum E'
    // DMP-NEXT:   effect: ACCParallelDirective
    // DMP-NEXT:     ACCNumGangsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     ACCCopyClause {{.*}} <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'enum E'
    // DMP-NEXT:     ACCReductionClause {{.*}} <implicit> 'max'
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'enum E'
    // DMP-NEXT:     impl: OMPTargetTeamsDirective
    // DMP-NEXT:       OMPNum_teamsClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:       OMPMapClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out' 'enum E'
    // DMP-NEXT:       OMPReductionClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out' 'enum E'
    // DMP:          ACCLoopDirective
    // DMP-NEXT:       ACCReductionClause {{.*}} 'max'
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out' 'enum E'
    // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:       ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:       impl: OMPDistributeDirective
    // DMP:              ForStmt
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel loop num_gangs(2) reduction(max: out){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) map(ompx_hold,tofrom: out) reduction(max: out){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
    //
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) map(ompx_hold,tofrom: out) reduction(max: out){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp distribute{{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) reduction(max: out){{$}}
    //
    // PRT-NEXT: for ({{.*}}) {
    #pragma acc parallel loop num_gangs(2) reduction(max: out)
    for (int i = 0; i < 2; ++i) {
      // DMP: ConditionalOperator
      // PRT-NEXT: out = E1 > out ? E1 : out;
      out = E1 > out ? E1 : out;
    } // PRT-NEXT: }
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: out = 1
    printf("out = %d\n", out);
  } // PRT-NEXT: }

  //----------------------------------------------------------------------------
  // auto with partitioning.
  //----------------------------------------------------------------------------

  // DMP-LABEL: StringLiteral {{.*}} "auto with partitioning\n"
  // PRT-LABEL: printf("auto with partitioning\n");
  //   EXE-NOT: {{.}}
  // EXE-LABEL: auto with partitioning{{$}}
  //   EXE-NOT: {{.}}
  printf("auto with partitioning\n");

  // Reduction var is private at loop.

  // FIXME: OpenMP offloading for nvptx64 doesn't store bool correctly for
  // reductions.
  // FIXME: Clang fails an assert here for amdgcn offloading.
// PRT-SRC-NEXT: #if !TGT_NVPTX64 && !TGT_AMDGCN
#if !TGT_NVPTX64 && !TGT_AMDGCN
  // PRT-NEXT: {
  {
    // PRT-NEXT: _Bool out0 = 1;
    // PRT-NEXT: _Bool out1 = 1;
    // PRT-NEXT: _Bool out2 = 1;
    _Bool out0 = 1;
    _Bool out1 = 1;
    _Bool out2 = 1;
    // DMP:      ACCParallelDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCReductionClause {{.*}} '&'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out0' '_Bool'
    // DMP-NEXT:   ACCFirstprivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out1' '_Bool'
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out2' '_Bool'
    // DMP-NEXT:   ACCNomapClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out1' '_Bool'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out2' '_Bool'
    // DMP-NEXT:   ACCCopyClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out0' '_Bool'
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out0' '_Bool'
    // DMP-NEXT:     OMPFirstprivateClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out1' '_Bool'
    // DMP-NEXT:     OMPPrivateClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out2' '_Bool'
    // DMP-NEXT:     OMPMapClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out0' '_Bool'
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(2) reduction(&: out0) firstprivate(out1) private(out2){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) reduction(&: out0) firstprivate(out1) private(out2) map(ompx_hold,tofrom: out0){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) reduction(&: out0) firstprivate(out1) private(out2) map(ompx_hold,tofrom: out0){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2) reduction(&: out0) firstprivate(out1) private(out2){{$}}
    #pragma acc parallel num_gangs(2) reduction(&: out0) firstprivate(out1) private(out2)
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      // PRT-NEXT: short in = 3;
      short in = 3;
      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCAutoClause
      // DMP-NEXT:   ACCWorkerClause
      // DMP-NEXT:   ACCReductionClause {{.*}} '&'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out0' '_Bool'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out1' '_Bool'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out2' '_Bool'
      // DMP-NEXT:   ACCReductionClause {{.*}} '|'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'in' 'short'
      // DMP-NEXT:   impl: ForStmt
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop auto worker reduction(&: out0,out1,out2) reduction(|: in)
      // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
      // PRT-A-SAME:  {{^$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop auto worker reduction(&: out0,out1,out2) reduction(|: in) // discarded in OpenMP translation{{$}}
      #pragma acc loop auto worker reduction(&: out0,out1,out2) reduction(|: in)
      // PRT-NEXT: for ({{.*}}) {
      for (int i = 0; i < 2; ++i) {
        // DMP: CompoundAssignOperator {{.*}} '_Bool' '&='
        // PRT-NEXT: out0 &= 0;
        out0 &= 0;
        // DMP: CompoundAssignOperator {{.*}} '_Bool' '&='
        // PRT-NEXT: out1 &= 0;
        out1 &= 0;
        // DMP: CompoundAssignOperator {{.*}} '_Bool' '&='
        // PRT-NEXT: out2 &= 0;
        out2 &= 0;
        // DMP: CompoundAssignOperator {{.*}} 'short' '|='
        // PRT-NEXT: in |= 10;
        in |= 10;
      } // PRT-NEXT: }
      // DMP: CallExpr
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // EXE-HOST-DAG: out1 = 0
      // EXE-HOST-DAG: out1 = 0
      // EXE-TGT-X86_64-DAG: out1 = 0
      // EXE-TGT-X86_64-DAG: out1 = 0
      // EXE-TGT-PPC64LE-DAG: out1 = 0
      // EXE-TGT-PPC64LE-DAG: out1 = 0
      TGT_PRINTF("out1 = %d\n", out1);
      // DMP: CallExpr
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // EXE-HOST-DAG: in: 11
      // EXE-HOST-DAG: in: 11
      // EXE-TGT-X86_64-DAG: in: 11
      // EXE-TGT-X86_64-DAG: in: 11
      // EXE-TGT-PPC64LE-DAG: in: 11
      // EXE-TGT-PPC64LE-DAG: in: 11
      TGT_PRINTF("in: %d\n", in);
    } // PRT-NEXT: }
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-HOST-NEXT: out0 = 0
    // EXE-TGT-X86_64-NEXT: out0 = 0
    // EXE-TGT-PPC64LE-NEXT: out0 = 0
    printf("out0 = %d\n", out0);
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-HOST-NEXT: out1 = 1
    // EXE-TGT-X86_64-NEXT: out1 = 1
    // EXE-TGT-PPC64LE-NEXT: out1 = 1
    printf("out1 = %d\n", out1);
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-HOST-NEXT: out2 = 1
    // EXE-TGT-X86_64-NEXT: out2 = 1
    // EXE-TGT-PPC64LE-NEXT: out2 = 1
    printf("out2 = %d\n", out2);
  } // PRT-NEXT: }
// PRT-SRC-NEXT: #endif
#endif

  // Reduction var is not private at loop due to explicit copy/copyin/copyout
  // clause.

  // PRT-NEXT: {
  {
    // PRT-NEXT: double out0 =
    // PRT-NEXT: double out1 =
    // PRT-NEXT: double out2 =
    double out0 = 0.3;
    double out1 = 1.4;
    double out2 = 2.5;
    // DMP:      ACCParallelDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCCopyClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out0' 'double'
    // DMP-NEXT:   ACCCopyinClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out1' 'double'
    // DMP-NEXT:   ACCCopyoutClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out2' 'double'
    // DMP-NEXT:   ACCReductionClause {{.*}} <implicit> '+'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out0' 'double'
    // DMP-NEXT:   ACCReductionClause {{.*}} <implicit> '+'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out1' 'double'
    // DMP-NEXT:   ACCReductionClause {{.*}} <implicit> '+'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out2' 'double'
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     OMPMapClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out0' 'double'
    // DMP-NEXT:     OMPMapClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out1' 'double'
    // DMP-NEXT:     OMPMapClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out2' 'double'
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out0' 'double'
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out1' 'double'
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out2' 'double'
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(2) pcopy(out0) pcopyin(out1) pcopyout(out2){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) map(ompx_hold,tofrom: out0) map(ompx_hold,to: out1) map(ompx_hold,from: out2) reduction(+: out0) reduction(+: out1) reduction(+: out2){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) map(ompx_hold,tofrom: out0) map(ompx_hold,to: out1) map(ompx_hold,from: out2) reduction(+: out0) reduction(+: out1) reduction(+: out2){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2) pcopy(out0) pcopyin(out1) pcopyout(out2){{$}}
    #pragma acc parallel num_gangs(2) pcopy(out0) pcopyin(out1) pcopyout(out2)
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      // Make sure accessing the variables before the reduction doesn't
      // produce a conflicting implicit firstprivate clause.
      // DMP: CallExpr
      // DMP: CallExpr
      // DMP: CallExpr
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // EXE-TGT-USE-STDIO-DAG: out0 init = 0.0{{$}}
      // EXE-TGT-USE-STDIO-DAG: out0 init = 0.0{{$}}
      // EXE-TGT-USE-STDIO-DAG: out1 init = 0.0{{$}}
      // EXE-TGT-USE-STDIO-DAG: out1 init = 0.0{{$}}
      // EXE-TGT-USE-STDIO-DAG: out2 init = 0.0{{$}}
      // EXE-TGT-USE-STDIO-DAG: out2 init = 0.0{{$}}
      TGT_PRINTF("out0 init = %.1f\n", out0);
      TGT_PRINTF("out1 init = %.1f\n", out1);
      TGT_PRINTF("out2 init = %.1f\n", out2);
      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCAutoClause
      // DMP-NEXT:   ACCWorkerClause
      // DMP-NEXT:   ACCVectorClause
      // DMP-NEXT:   ACCReductionClause {{.*}} '+'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out0' 'double'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out1' 'double'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out2' 'double'
      // DMP-NEXT:   impl: ForStmt
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop auto worker vector reduction(+: out0,out1,out2)
      // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
      // PRT-A-SAME:  {{^$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop auto worker vector reduction(+: out0,out1,out2) // discarded in OpenMP translation{{$}}
      #pragma acc loop auto worker vector reduction(+: out0,out1,out2)
      // PRT-NEXT: for ({{.*}}) {
      for (int i = 0; i < 4; ++i) {
        // DMP: CompoundAssignOperator {{.*}} '+='
        // DMP: CompoundAssignOperator {{.*}} '+='
        // DMP: CompoundAssignOperator {{.*}} '+='
        // PRT-NEXT: out0 +=
        // PRT-NEXT: out1 +=
        // PRT-NEXT: out2 +=
        out0 += -1.1;
        out1 += -1.1;
        out2 += -1.1;
      } // PRT-NEXT: }
    } // PRT-NEXT: }
    // DMP: CallExpr
    // DMP: CallExpr
    // DMP: CallExpr
    // PRT-NEXT: printf
    // PRT-NEXT: printf
    // PRT-NEXT: printf
    // EXE-NEXT: out0 = -8.5
    // EXE-HOST-NEXT: out1 = -7.4
    // EXE-HOST-NEXT: out2 = -6.3
    // EXE-TGT-X86_64-NEXT: out1 = 1.4
    // EXE-TGT-X86_64-NEXT: out2 = {{.*}}
    // EXE-TGT-PPC64LE-NEXT: out1 = 1.4
    // EXE-TGT-PPC64LE-NEXT: out2 = {{.*}}
    // EXE-TGT-NVPTX64-NEXT: out1 = 1.4
    // EXE-TGT-NVPTX64-NEXT: out2 = {{.*}}
    // EXE-TGT-AMDGCN-NEXT: out1 = 1.4
    // EXE-TGT-AMDGCN-NEXT: out2 = {{.*}}
    printf("out0 = %.1f\n", out0);
    printf("out1 = %.1f\n", out1);
    printf("out2 = %.1f\n", out2);
  } // PRT-NEXT: }

  // Reduction var is not private at loop due to copy clause implied by
  // reduction on combined construct.

// PRT-SRC-NEXT: #if !TGT_NVPTX64 && !TGT_AMDGCN
#if !TGT_NVPTX64 && !TGT_AMDGCN
  // PRT-NEXT: {
  {
    // PRT-NEXT: _Bool out = 1;
    _Bool out = 1;
    // DMP:      ACCParallelLoopDirective
    // DMP-NEXT:   ACCReductionClause {{.*}} '&'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out' '_Bool'
    // DMP-NEXT:   ACCAutoClause
    // DMP-NEXT:   ACCWorkerClause
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   effect: ACCParallelDirective
    // DMP-NEXT:     ACCNumGangsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     ACCCopyClause {{.*}} <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' '_Bool'
    // DMP-NEXT:     ACCReductionClause {{.*}} <implicit> '&'
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' '_Bool'
    // DMP-NEXT:     impl: OMPTargetTeamsDirective
    // DMP-NEXT:       OMPNum_teamsClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:       OMPMapClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out' '_Bool'
    // DMP-NEXT:       OMPReductionClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out' '_Bool'
    // DMP:          ACCLoopDirective
    // DMP-NEXT:       ACCReductionClause {{.*}} '&'
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out' '_Bool'
    // DMP-NEXT:       ACCAutoClause
    // DMP-NEXT:       ACCWorkerClause
    // DMP-NEXT:       impl: ForStmt
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel loop reduction(&: out) auto worker num_gangs(2){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) map(ompx_hold,tofrom: out) reduction(&: out){{$}}
    //
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) map(ompx_hold,tofrom: out) reduction(&: out){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop reduction(&: out) auto worker num_gangs(2){{$}}
    //
    // PRT-NEXT: for ({{.*}}) {
    #pragma acc parallel loop reduction(&: out) auto worker num_gangs(2)
    for (int i = 0; i < 2; ++i) {
      // DMP: CompoundAssignOperator {{.*}} '_Bool' '&='
      // PRT-NEXT: out &= 0;
      out &= 0;
    } // PRT-NEXT: }
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-HOST-NEXT: out = 0
    // EXE-TGT-X86_64-NEXT: out = 0
    // EXE-TGT-PPC64LE-NEXT: out = 0
    printf("out = %d\n", out);
  } // PRT-NEXT: }
// PRT-SRC-NEXT: #endif
#endif

  //----------------------------------------------------------------------------
  // worker and implicit gang.
  //
  // FIXME: OpenMP offloading nvptx64 doesn't seem to support _Complex
  // properly.  When the host is x86_64, we see incorrect values printed within
  // the following kernel.  When the host is ppc64le, the following produces a
  // runtime error:
  //
  //   Libomptarget fatal error 1: failure of target construct while
  //   offloading is mandatory
  //
  // FIXME: We couldn't include complex.h for amdgcn (see above).
  //----------------------------------------------------------------------------

  // DMP-LABEL: StringLiteral {{.*}} "worker and implicit gang\n"
  // PRT-LABEL: printf("worker and implicit gang\n");
  //   EXE-NOT: {{.}}
  // EXE-LABEL: worker and implicit gang{{$}}
  //   EXE-NOT: {{.}}
  printf("worker and implicit gang\n");

  // Reduction vars are private at the loop.

// PRT-SRC-NEXT: #if !TGT_NVPTX64 && !TGT_AMDGCN
#if !TGT_NVPTX64 && !TGT_AMDGCN
  // PRT-NEXT: {
  {
    // PRT-NEXT: _Complex double out0 = 2;
    // PRT-NEXT: _Complex double out1 = 4;
    // PRT-NEXT: _Complex double out2 = 5;
    _Complex double out0 = 2;
    _Complex double out1 = 4;
    _Complex double out2 = 5;
    // DMP:      ACCParallelDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCReductionClause {{.*}} '&&'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out0' '_Complex double'
    // DMP-NEXT:   ACCFirstprivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out1' '_Complex double'
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out2' '_Complex double'
    // DMP-NEXT:   ACCNomapClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out1' '_Complex double'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out2' '_Complex double'
    // DMP-NEXT:   ACCCopyClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out0' '_Complex double'
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out0' '_Complex double'
    // DMP-NEXT:     OMPFirstprivateClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out1' '_Complex double'
    // DMP-NEXT:     OMPPrivateClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out2' '_Complex double'
    // DMP-NEXT:     OMPMapClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out0' '_Complex double'
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(2) reduction(&&: out0) firstprivate(out1) private(out2){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) reduction(&&: out0) firstprivate(out1) private(out2) map(ompx_hold,tofrom: out0){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) reduction(&&: out0) firstprivate(out1) private(out2) map(ompx_hold,tofrom: out0){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2) reduction(&&: out0) firstprivate(out1) private(out2){{$}}
    #pragma acc parallel num_gangs(2) reduction(&&: out0) firstprivate(out1) private(out2)
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      // PRT-NEXT: double in = 0;
      double in = 0;
      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCWorkerClause
      // DMP-NEXT:   ACCReductionClause {{.*}} '&&'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out0' '_Complex double'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out1' '_Complex double'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out2' '_Complex double'
      // DMP-NEXT:   ACCReductionClause {{.*}} '||'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'in' 'double'
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
      // DMP-NEXT:   impl: OMPDistributeParallelForDirective
      // DMP-NEXT:     OMPReductionClause
      // DMP-NEXT:       DeclRefExpr {{.*}} 'out0' '_Complex double'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'out1' '_Complex double'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'out2' '_Complex double'
      // DMP-NEXT:     OMPReductionClause
      // DMP-NEXT:       DeclRefExpr {{.*}} 'in' 'double'
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop worker reduction(&&: out0,out1,out2) reduction(||: in){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for reduction(&&: out0,out1,out2) reduction(||: in){{$}}
      // PRT-O-NEXT:  {{^ *}}#pragma omp distribute parallel for reduction(&&: out0,out1,out2) reduction(||: in){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker reduction(&&: out0,out1,out2) reduction(||: in){{$}}
      #pragma acc loop worker reduction(&&: out0,out1,out2) reduction(||: in)
      // PRT-NEXT: for ({{.*}}) {
      for (int i = 0; i < 2; ++i) {
        // DMP: BinaryOperator {{.*}} '&&'
        // PRT-NEXT: out0 = out0 && 0;
        out0 = out0 && 0;
        // DMP: BinaryOperator {{.*}} '&&'
        // PRT-NEXT: out1 = out1 && 0;
        out1 = out1 && 0;
        // DMP: BinaryOperator {{.*}} '&&'
        // PRT-NEXT: out2 = out2 && 0;
        out2 = out2 && 0;
        // DMP: BinaryOperator {{.*}} '||'
        // PRT-NEXT: in = in || 10;
        in = in || 10;
      } // PRT-NEXT: }
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // EXE-HOST-DAG: out1: 0.0 + 0.0i
      // EXE-HOST-DAG: out1: 0.0 + 0.0i
      // EXE-TGT-X86_64-DAG: out1: 0.0 + 0.0i
      // EXE-TGT-X86_64-DAG: out1: 0.0 + 0.0i
      // EXE-TGT-PPC64LE-DAG: out1: 0.0 + 0.0i
      // EXE-TGT-PPC64LE-DAG: out1: 0.0 + 0.0i
      TGT_PRINTF("out1: %.1f + %.1fi\n", creal(out1), cimag(out1));
      // DMP: CallExpr
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      //        EXE-HOST-DAG: in: 1.0
      //        EXE-HOST-DAG: in: 1.0
      //  EXE-TGT-X86_64-DAG: in: 1.0
      //  EXE-TGT-X86_64-DAG: in: 1.0
      // EXE-TGT-PPC64LE-DAG: in: 1.0
      // EXE-TGT-PPC64LE-DAG: in: 1.0
      TGT_PRINTF("in: %.1f\n", in);
    } // PRT-NEXT: }
    // DMP: CallExpr
    // PRT-NEXT: printf
    //        EXE-HOST-NEXT: out0 = 0.0 + 0.0i
    //  EXE-TGT-X86_64-NEXT: out0 = 0.0 + 0.0i
    // EXE-TGT-PPC64LE-NEXT: out0 = 0.0 + 0.0i
    printf("out0 = %.1f + %.1fi\n", creal(out0), cimag(out0));
    // DMP: CallExpr
    // PRT-NEXT: printf
    //        EXE-HOST-NEXT: out1 = 4.0 + 0.0i
    //  EXE-TGT-X86_64-NEXT: out1 = 4.0 + 0.0i
    // EXE-TGT-PPC64LE-NEXT: out1 = 4.0 + 0.0i
    printf("out1 = %.1f + %.1fi\n", creal(out1), cimag(out1));
    // DMP: CallExpr
    // PRT-NEXT: printf
    //        EXE-HOST-NEXT: out2 = 5.0 + 0.0i
    //  EXE-TGT-X86_64-NEXT: out2 = 5.0 + 0.0i
    // EXE-TGT-PPC64LE-NEXT: out2 = 5.0 + 0.0i
    printf("out2 = %.1f + %.1fi\n", creal(out2), cimag(out2));
  } // PRT-NEXT: }
// PRT-SRC-NEXT: #endif
#endif

  // Reduction var is not private at loop due to explicit copy/copyin/copyout
  // clause.

  // PRT-NEXT: {
  {
    // PRT-NEXT: double out0 =
    // PRT-NEXT: double out1 =
    // PRT-NEXT: double out2 =
    double out0 = 0.3;
    double out1 = 1.4;
    double out2 = 2.5;
    // DMP:      ACCParallelDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCCopyClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out0' 'double'
    // DMP-NEXT:   ACCCopyinClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out1' 'double'
    // DMP-NEXT:   ACCCopyoutClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out2' 'double'
    // DMP-NEXT:   ACCReductionClause {{.*}} <implicit> '+'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out0' 'double'
    // DMP-NEXT:   ACCReductionClause {{.*}} <implicit> '+'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out1' 'double'
    // DMP-NEXT:   ACCReductionClause {{.*}} <implicit> '+'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out2' 'double'
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     OMPMapClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out0' 'double'
    // DMP-NEXT:     OMPMapClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out1' 'double'
    // DMP-NEXT:     OMPMapClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out2' 'double'
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out0' 'double'
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out1' 'double'
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out2' 'double'
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(2) present_or_copy(out0) present_or_copyin(out1) present_or_copyout(out2){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) map(ompx_hold,tofrom: out0) map(ompx_hold,to: out1) map(ompx_hold,from: out2) reduction(+: out0) reduction(+: out1) reduction(+: out2){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) map(ompx_hold,tofrom: out0) map(ompx_hold,to: out1) map(ompx_hold,from: out2) reduction(+: out0) reduction(+: out1) reduction(+: out2){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2) present_or_copy(out0) present_or_copyin(out1) present_or_copyout(out2){{$}}
    #pragma acc parallel num_gangs(2) present_or_copy(out0) present_or_copyin(out1) present_or_copyout(out2)
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      // Make sure accessing the variables before the reduction doesn't
      // produce a conflicting implicit firstprivate clause.
      // DMP: CallExpr
      // DMP: CallExpr
      // DMP: CallExpr
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // EXE-TGT-USE-STDIO-DAG: out0 init = 0.0{{$}}
      // EXE-TGT-USE-STDIO-DAG: out0 init = 0.0{{$}}
      // EXE-TGT-USE-STDIO-DAG: out1 init = 0.0{{$}}
      // EXE-TGT-USE-STDIO-DAG: out1 init = 0.0{{$}}
      // EXE-TGT-USE-STDIO-DAG: out2 init = 0.0{{$}}
      // EXE-TGT-USE-STDIO-DAG: out2 init = 0.0{{$}}
      TGT_PRINTF("out0 init = %.1f\n", out0);
      TGT_PRINTF("out1 init = %.1f\n", out1);
      TGT_PRINTF("out2 init = %.1f\n", out2);
      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCWorkerClause
      // DMP-NEXT:   ACCReductionClause {{.*}} '+'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out0' 'double'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out1' 'double'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out2' 'double'
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
      // DMP-NEXT:   impl: OMPDistributeParallelForDirective
      // DMP-NEXT:     OMPReductionClause
      // DMP-NEXT:       DeclRefExpr {{.*}} 'out0' 'double'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'out1' 'double'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'out2' 'double'
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop worker reduction(+: out0,out1,out2){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for reduction(+: out0,out1,out2){{$}}
      // PRT-O-NEXT:  {{^ *}}#pragma omp distribute parallel for reduction(+: out0,out1,out2){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker reduction(+: out0,out1,out2){{$}}
      #pragma acc loop worker reduction(+: out0,out1,out2)
      // PRT-NEXT: for ({{.*}}) {
      for (int i = 0; i < 4; ++i) {
        // DMP: CompoundAssignOperator {{.*}} '+='
        // DMP: CompoundAssignOperator {{.*}} '+='
        // DMP: CompoundAssignOperator {{.*}} '+='
        // PRT-NEXT: out0 +=
        // PRT-NEXT: out1 +=
        // PRT-NEXT: out2 +=
        out0 += -1.1;
        out1 += -1.1;
        out2 += -1.1;
      } // PRT-NEXT: }
    } // PRT-NEXT: }
    // DMP: CallExpr
    // DMP: CallExpr
    // DMP: CallExpr
    // PRT-NEXT: printf
    // PRT-NEXT: printf
    // PRT-NEXT: printf
    // EXE-HOST-NEXT: out0 = -4.1{{$}}
    // EXE-HOST-NEXT: out1 = -3.0{{$}}
    // EXE-HOST-NEXT: out2 = -1.9{{$}}
    //  EXE-OFF-NEXT: out0 = -4.1{{$}}
    //  EXE-OFF-NEXT: out1 = 1.4{{$}}
    //  EXE-OFF-NEXT: out2 = {{.*}}
    printf("out0 = %.1f\n", out0);
    printf("out1 = %.1f\n", out1);
    printf("out2 = %.1f\n", out2);
  } // PRT-NEXT: }

  // Reduction var is not private at loop due to copy clause implied by
  // reduction on combined construct.

  // FIXME: We couldn't include complex.h for amdgcn (see above).
// PRT-SRC-NEXT: #if !TGT_AMDGCN
#if !TGT_AMDGCN
  // PRT-NEXT: {
  {
    // PRT-NEXT: _Complex double out = 2;
    _Complex double out = 2;
    // DMP:      ACCParallelLoopDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCWorkerClause
    // DMP-NEXT:   ACCReductionClause {{.*}} '&&'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out' '_Complex double'
    // DMP-NEXT:   effect: ACCParallelDirective
    // DMP-NEXT:     ACCNumGangsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     ACCCopyClause {{.*}} <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' '_Complex double'
    // DMP-NEXT:     ACCReductionClause {{.*}} <implicit> '&&'
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' '_Complex double'
    // DMP-NEXT:     impl: OMPTargetTeamsDirective
    // DMP-NEXT:       OMPNum_teamsClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:       OMPMapClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out' '_Complex double'
    // DMP-NEXT:       OMPReductionClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out' '_Complex double'
    // DMP:          ACCLoopDirective
    // DMP-NEXT:       ACCWorkerClause
    // DMP-NEXT:       ACCReductionClause {{.*}} '&&'
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out' '_Complex double'
    // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:       ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:       impl: OMPDistributeParallelForDirective
    // DMP-NEXT:         OMPReductionClause
    // DMP-NEXT:           DeclRefExpr {{.*}} 'out' '_Complex double'
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel loop num_gangs(2) worker reduction(&&: out){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) map(ompx_hold,tofrom: out) reduction(&&: out){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for reduction(&&: out){{$}}
    //
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) map(ompx_hold,tofrom: out) reduction(&&: out){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp distribute parallel for reduction(&&: out){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) worker reduction(&&: out){{$}}
    #pragma acc parallel loop num_gangs(2) worker reduction(&&: out)
    // PRT-NEXT: for ({{.*}}) {
    for (int i = 0; i < 2; ++i) {
      // DMP: BinaryOperator {{.*}} '&&'
      // PRT-NEXT: out = out && 0;
      out = out && 0;
    } // PRT-NEXT: }
    // DMP: CallExpr
    // PRT-NEXT: printf
    //        EXE-HOST-NEXT: out = 0.0 + 0.0i
    //  EXE-TGT-X86_64-NEXT: out = 0.0 + 0.0i
    // EXE-TGT-PPC64LE-NEXT: out = 0.0 + 0.0i
    // EXE-TGT-NVPTX64-NEXT: out = 0.0 + 0.0i
    printf("out = %.1f + %.1fi\n", creal(out), cimag(out));
  } // PRT-NEXT: }
// PRT-SRC-NEXT: #endif
#endif

  //----------------------------------------------------------------------------
  // vector and implicit gang.
  //----------------------------------------------------------------------------

  // DMP-LABEL: StringLiteral {{.*}} "vector and implicit gang\n"
  // PRT-LABEL: printf("vector and implicit gang\n");
  //   EXE-NOT: {{.}}
  // EXE-LABEL: vector and implicit gang{{$}}
  //   EXE-NOT: {{.}}
  printf("vector and implicit gang\n");

  // Reduction vars are private at the loop.
  //
  // OpenMP offloading to nvptx64 isn't supported for long double.
  // OpenMP offloading to ppc64le isn't supported for long double.
  // OpenMP offloading to amdgcn isn't supported for long double.

// PRT-SRC-NEXT: #if !TGT_PPC64LE && !TGT_NVPTX64 && !TGT_AMDGCN
#if !TGT_PPC64LE && !TGT_NVPTX64 && !TGT_AMDGCN
  // PRT-NEXT: {
  {
    // PRT-NEXT: long out0 = 5;
    // PRT-NEXT: long out1 = 7;
    // PRT-NEXT: long out2 = 8;
    long out0 = 5;
    long out1 = 7;
    long out2 = 8;
    // DMP:      ACCParallelDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCReductionClause {{.*}} '+'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out0' 'long'
    // DMP-NEXT:   ACCFirstprivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out1' 'long'
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out2' 'long'
    // DMP-NEXT:   ACCNomapClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out1' 'long'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out2' 'long'
    // DMP-NEXT:   ACCCopyClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out0' 'long'
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out0' 'long'
    // DMP-NEXT:     OMPFirstprivateClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out1' 'long'
    // DMP-NEXT:     OMPPrivateClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out2' 'long'
    // DMP-NEXT:     OMPMapClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out0' 'long'
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(2) reduction(+: out0) firstprivate(out1) private(out2){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) reduction(+: out0) firstprivate(out1) private(out2) map(ompx_hold,tofrom: out0){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) reduction(+: out0) firstprivate(out1) private(out2) map(ompx_hold,tofrom: out0){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2) reduction(+: out0) firstprivate(out1) private(out2){{$}}
    #pragma acc parallel num_gangs(2) reduction(+: out0) firstprivate(out1) private(out2)
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      // PRT-NEXT: long double in = -1;
      long double in = -1;
      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCVectorClause
      // DMP-NEXT:   ACCReductionClause {{.*}} '+
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out0' 'long'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out1' 'long'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out2' 'long'
      // DMP-NEXT:   ACCReductionClause {{.*}} '*
      // DMP-NEXT:     DeclRefExpr {{.*}} 'in' 'long double'
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
      // DMP-NEXT:   impl: OMPDistributeSimdDirective
      // DMP-NEXT:     OMPReductionClause
      // DMP-NEXT:       DeclRefExpr {{.*}} 'out0' 'long'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'out1' 'long'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'out2' 'long'
      // DMP-NEXT:     OMPReductionClause
      // DMP-NEXT:       DeclRefExpr {{.*}} 'in' 'long double'
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop vector reduction(+: out0,out1,out2) reduction(*: in){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute simd reduction(+: out0,out1,out2) reduction(*: in){{$}}
      // PRT-O-NEXT:  {{^ *}}#pragma omp distribute simd reduction(+: out0,out1,out2) reduction(*: in){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop vector reduction(+: out0,out1,out2) reduction(*: in){{$}}
      #pragma acc loop vector reduction(+: out0,out1,out2) reduction(*: in)
      // PRT-NEXT: for ({{.*}}) {
      for (int i = 0; i < 2; ++i) {
        // DMP: CompoundAssignOperator {{.*}} '+='
        // PRT-NEXT: out0 += 10;
        out0 += 10;
        // DMP: CompoundAssignOperator {{.*}} '+='
        // PRT-NEXT: out1 += 10;
        out1 += 10;
        // DMP: CompoundAssignOperator {{.*}} '+='
        // PRT-NEXT: out2 += 10;
        out2 += 10;
        // DMP: CompoundAssignOperator {{.*}} '*='
        // PRT-NEXT: in *= 3;
        in *= 3;
      } // PRT-NEXT: }
      // DMP: CallExpr
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      //       EXE-HOST-DAG: out1 = 17
      //       EXE-HOST-DAG: out1 = 17
      // EXE-TGT-X86_64-DAG: out1 = 17
      // EXE-TGT-X86_64-DAG: out1 = 17
      TGT_PRINTF("out1 = %ld\n", out1);
      // DMP: CallExpr
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      //       EXE-HOST-DAG: in: -3.0
      //       EXE-HOST-DAG: in: -3.0
      // EXE-TGT-X86_64-DAG: in: -3.0
      // EXE-TGT-X86_64-DAG: in: -3.0
      TGT_PRINTF("in: %.1f\n", (double)in);
    } // PRT-NEXT: }
    // DMP: CallExpr
    // PRT-NEXT: printf
    //       EXE-HOST-NEXT: out0 = 25
    // EXE-TGT-X86_64-NEXT: out0 = 25
    printf("out0 = %ld\n", out0);
  } // PRT-NEXT: }
// PRT-SRC-NEXT: #endif
#endif

  // Reduction var is not private at loop due to explicit copy/copyin/copyout
  // clause.

  // PRT-NEXT: {
  {
    // PRT-NEXT: double out0 =
    // PRT-NEXT: double out1 =
    // PRT-NEXT: double out2 =
    double out0 = 0.3;
    double out1 = 1.4;
    double out2 = 2.5;
    // DMP:      ACCParallelDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCCopyClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out0' 'double'
    // DMP-NEXT:   ACCCopyinClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out1' 'double'
    // DMP-NEXT:   ACCCopyoutClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out2' 'double'
    // DMP-NEXT:   ACCReductionClause {{.*}} <implicit> '+'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out0' 'double'
    // DMP-NEXT:   ACCReductionClause {{.*}} <implicit> '+'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out1' 'double'
    // DMP-NEXT:   ACCReductionClause {{.*}} <implicit> '+'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out2' 'double'
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     OMPMapClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out0' 'double'
    // DMP-NEXT:     OMPMapClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out1' 'double'
    // DMP-NEXT:     OMPMapClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out2' 'double'
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out0' 'double'
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out1' 'double'
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out2' 'double'
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(2) copy(out0) copyin(out1) copyout(out2){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) map(ompx_hold,tofrom: out0) map(ompx_hold,to: out1) map(ompx_hold,from: out2) reduction(+: out0) reduction(+: out1) reduction(+: out2){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) map(ompx_hold,tofrom: out0) map(ompx_hold,to: out1) map(ompx_hold,from: out2) reduction(+: out0) reduction(+: out1) reduction(+: out2){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2) copy(out0) copyin(out1) copyout(out2){{$}}
    #pragma acc parallel num_gangs(2) copy(out0) copyin(out1) copyout(out2)
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      // Make sure accessing the variables before the reduction doesn't
      // produce a conflicting implicit firstprivate clause.
      // DMP: CallExpr
      // DMP: CallExpr
      // DMP: CallExpr
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // EXE-TGT-USE-STDIO-DAG: out0 init = 0.0{{$}}
      // EXE-TGT-USE-STDIO-DAG: out0 init = 0.0{{$}}
      // EXE-TGT-USE-STDIO-DAG: out1 init = 0.0{{$}}
      // EXE-TGT-USE-STDIO-DAG: out1 init = 0.0{{$}}
      // EXE-TGT-USE-STDIO-DAG: out2 init = 0.0{{$}}
      // EXE-TGT-USE-STDIO-DAG: out2 init = 0.0{{$}}
      TGT_PRINTF("out0 init = %.1f\n", out0);
      TGT_PRINTF("out1 init = %.1f\n", out1);
      TGT_PRINTF("out2 init = %.1f\n", out2);
      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCVectorClause
      // DMP-NEXT:   ACCReductionClause {{.*}} '+'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out0' 'double'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out1' 'double'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out2' 'double'
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
      // DMP-NEXT:   impl: OMPDistributeSimdDirective
      // DMP-NEXT:     OMPReductionClause
      // DMP-NEXT:       DeclRefExpr {{.*}} 'out0' 'double'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'out1' 'double'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'out2' 'double'
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop vector reduction(+: out0,out1,out2){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute simd reduction(+: out0,out1,out2){{$}}
      // PRT-O-NEXT:  {{^ *}}#pragma omp distribute simd reduction(+: out0,out1,out2){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop vector reduction(+: out0,out1,out2){{$}}
      #pragma acc loop vector reduction(+: out0,out1,out2)
      // PRT-NEXT: for ({{.*}}) {
      for (int i = 0; i < 4; ++i) {
        // DMP: CompoundAssignOperator {{.*}} '+='
        // DMP: CompoundAssignOperator {{.*}} '+='
        // DMP: CompoundAssignOperator {{.*}} '+='
        // PRT-NEXT: out0 +=
        // PRT-NEXT: out1 +=
        // PRT-NEXT: out2 +=
        out0 += -1.1;
        out1 += -1.1;
        out2 += -1.1;
      } // PRT-NEXT: }
    } // PRT-NEXT: }
    // DMP: CallExpr
    // DMP: CallExpr
    // DMP: CallExpr
    // PRT-NEXT: printf
    // PRT-NEXT: printf
    // PRT-NEXT: printf
    // EXE-NEXT: out0 = -4.1
    //        EXE-HOST-NEXT: out1 = -3.0
    //        EXE-HOST-NEXT: out2 = -1.9
    //  EXE-TGT-X86_64-NEXT: out1 = 1.4
    //  EXE-TGT-X86_64-NEXT: out2 =
    // EXE-TGT-PPC64LE-NEXT: out1 = 1.4
    // EXE-TGT-PPC64LE-NEXT: out2 =
    // EXE-TGT-NVPTX64-NEXT: out1 = 1.4
    // EXE-TGT-NVPTX64-NEXT: out2 =
    //  EXE-TGT-AMDGCN-NEXT: out1 = 1.4
    //  EXE-TGT-AMDGCN-NEXT: out2 =
    printf("out0 = %.1f\n", out0);
    printf("out1 = %.1f\n", out1);
    printf("out2 = %.1f\n", out2);
  } // PRT-NEXT: }

  // Reduction var is not private at loop due to copy clause implied by
  // reduction on combined construct.

  // PRT-NEXT: {
  {
    // PRT-NEXT: long out = 5;
    long out = 5;
    // DMP:      ACCParallelLoopDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCReductionClause {{.*}} '+'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'long'
    // DMP-NEXT:   ACCVectorClause
    // DMP-NEXT:   effect: ACCParallelDirective
    // DMP-NEXT:     ACCNumGangsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     ACCCopyClause {{.*}} <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'long'
    // DMP-NEXT:     ACCReductionClause {{.*}} <implicit> '+'
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'long'
    // DMP-NEXT:     impl: OMPTargetTeamsDirective
    // DMP-NEXT:       OMPNum_teamsClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:       OMPMapClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out' 'long'
    // DMP-NEXT:       OMPReductionClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out' 'long'
    // DMP:          ACCLoopDirective
    // DMP-NEXT:       ACCReductionClause {{.*}} '+
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out' 'long'
    // DMP-NEXT:       ACCVectorClause
    // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:       ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:       impl: OMPDistributeSimdDirective
    // DMP-NEXT:         OMPReductionClause
    // DMP-NEXT:           DeclRefExpr {{.*}} 'out' 'long'
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel loop num_gangs(2) reduction(+: out) vector{{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) map(ompx_hold,tofrom: out) reduction(+: out){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute simd reduction(+: out){{$}}
    //
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) map(ompx_hold,tofrom: out) reduction(+: out){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp distribute simd reduction(+: out){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) reduction(+: out) vector{{$}}
    #pragma acc parallel loop num_gangs(2) reduction(+: out) vector
    // PRT-NEXT: for ({{.*}}) {
    for (int i = 0; i < 2; ++i) {
      // DMP: CompoundAssignOperator {{.*}} '+='
      // PRT-NEXT: out += 10;
      out += 10;
    } // PRT-NEXT: }
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: out = 25
    printf("out = %ld\n", out);
  } // PRT-NEXT: }

  //----------------------------------------------------------------------------
  // worker, vector, and implicit gang.
  //----------------------------------------------------------------------------

  // DMP-LABEL: StringLiteral {{.*}} "worker, vector, and implicit gang\n"
  // PRT-LABEL: printf("worker, vector, and implicit gang\n");
  //   EXE-NOT: {{.}}
  // EXE-LABEL: worker, vector, and implicit gang{{$}}
  //   EXE-NOT: {{.}}
  printf("worker, vector, and implicit gang\n");

  // Reduction vars are private at the loop.

  // PRT-NEXT: {
  {
    // PRT-NEXT: float out0 = -5;
    // PRT-NEXT: float out1 = -7;
    // PRT-NEXT: float out2 = -8;
    float out0 = -5;
    float out1 = -7;
    float out2 = -8;
    // DMP:      ACCParallelDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCReductionClause {{.*}} '*'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out0' 'float'
    // DMP-NEXT:   ACCFirstprivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out1' 'float'
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out2' 'float'
    // DMP-NEXT:   ACCNomapClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out1' 'float'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out2' 'float'
    // DMP-NEXT:   ACCCopyClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out0' 'float'
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out0' 'float'
    // DMP-NEXT:     OMPFirstprivateClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out1' 'float'
    // DMP-NEXT:     OMPPrivateClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out2' 'float'
    // DMP-NEXT:     OMPMapClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out0' 'float'
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(2) reduction(*: out0) firstprivate(out1) private(out2){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) reduction(*: out0) firstprivate(out1) private(out2) map(ompx_hold,tofrom: out0){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) reduction(*: out0) firstprivate(out1) private(out2) map(ompx_hold,tofrom: out0){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2) reduction(*: out0) firstprivate(out1) private(out2){{$}}
    #pragma acc parallel num_gangs(2) reduction(*: out0) firstprivate(out1) private(out2)
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      // PRT-NEXT: int in = 3;
      int in = 3;
      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCWorkerClause
      // DMP-NEXT:   ACCVectorClause
      // DMP-NEXT:   ACCReductionClause {{.*}} '*'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out0' 'float'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out1' 'float'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out2' 'float'
      // DMP-NEXT:   ACCReductionClause {{.*}} '+'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'in' 'int'
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
      // DMP-NEXT:   impl: OMPDistributeParallelForSimdDirective
      // DMP-NEXT:     OMPReductionClause
      // DMP-NEXT:       DeclRefExpr {{.*}} 'out0' 'float'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'out1' 'float'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'out2' 'float'
      // DMP-NEXT:     OMPReductionClause
      // DMP-NEXT:       DeclRefExpr {{.*}} 'in' 'int'
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop worker vector reduction(*: out0,out1,out2) reduction(+: in){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for simd reduction(*: out0,out1,out2) reduction(+: in){{$}}
      // PRT-O-NEXT:  {{^ *}}#pragma omp distribute parallel for simd reduction(*: out0,out1,out2) reduction(+: in){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker vector reduction(*: out0,out1,out2) reduction(+: in){{$}}
      #pragma acc loop worker vector reduction(*: out0,out1,out2) reduction(+: in)
      // PRT-NEXT: for ({{.*}}) {
      for (int i = 0; i < 4; ++i) {
        // DMP: CompoundAssignOperator {{.*}} '*='
        // PRT-NEXT: out0 *= 3;
        out0 *= 3;
        // DMP: CompoundAssignOperator {{.*}} '*='
        // PRT-NEXT: out1 *= 3;
        out1 *= 3;
        // DMP: CompoundAssignOperator {{.*}} '*='
        // PRT-NEXT: out2 *= 3;
        out2 *= 3;
        // DMP: CompoundAssignOperator {{.*}} '+='
        // PRT-NEXT: in += 4;
        in += 4;
      } // PRT-NEXT: }
      // DMP: CallExpr
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // EXE-TGT-USE-STDIO-DAG: out1 = -63.0{{$}}
      TGT_PRINTF("out1 = %.1f\n", out1);
      // DMP: CallExpr
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // EXE-TGT-USE-STDIO-DAG: in: 11{{$}}
      // EXE-TGT-USE-STDIO-DAG: in: 11{{$}}
      TGT_PRINTF("in: %d\n", in);
    } // PRT-NEXT: }
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: out0 = -405.0{{$}}
    printf("out0 = %.1f\n", out0);
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: out1 = -7.0{{$}}
    printf("out1 = %.1f\n", out1);
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: out2 = -8.0{{$}}
    printf("out2 = %.1f\n", out2);
  } // PRT-NEXT: }

  // Reduction var is not private at loop due to explicit copy/copyin/copyout
  // clause.

  // PRT-NEXT: {
  {
    // PRT-NEXT: double out0 =
    // PRT-NEXT: double out1 =
    // PRT-NEXT: double out2 =
    double out0 = 0.3;
    double out1 = 1.4;
    double out2 = 2.5;
    // DMP:      ACCParallelDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCCopyClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out0' 'double'
    // DMP-NEXT:   ACCCopyinClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out1' 'double'
    // DMP-NEXT:   ACCCopyoutClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out2' 'double'
    // DMP-NEXT:   ACCReductionClause {{.*}} <implicit> '+'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out0' 'double'
    // DMP-NEXT:   ACCReductionClause {{.*}} <implicit> '+'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out1' 'double'
    // DMP-NEXT:   ACCReductionClause {{.*}} <implicit> '+'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out2' 'double'
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     OMPMapClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out0' 'double'
    // DMP-NEXT:     OMPMapClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out1' 'double'
    // DMP-NEXT:     OMPMapClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out2' 'double'
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out0' 'double'
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out1' 'double'
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out2' 'double'
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(2) pcopy(out0) pcopyin(out1) pcopyout(out2){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) map(ompx_hold,tofrom: out0) map(ompx_hold,to: out1) map(ompx_hold,from: out2) reduction(+: out0) reduction(+: out1) reduction(+: out2){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) map(ompx_hold,tofrom: out0) map(ompx_hold,to: out1) map(ompx_hold,from: out2) reduction(+: out0) reduction(+: out1) reduction(+: out2){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2) pcopy(out0) pcopyin(out1) pcopyout(out2){{$}}
    #pragma acc parallel num_gangs(2) pcopy(out0) pcopyin(out1) pcopyout(out2)
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      // Make sure accessing the variables before the reduction doesn't
      // produce a conflicting implicit firstprivate clause.
      // DMP: CallExpr
      // DMP: CallExpr
      // DMP: CallExpr
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // EXE-TGT-USE-STDIO-DAG: out0 init = 0.0{{$}}
      // EXE-TGT-USE-STDIO-DAG: out0 init = 0.0{{$}}
      // EXE-TGT-USE-STDIO-DAG: out1 init = 0.0{{$}}
      // EXE-TGT-USE-STDIO-DAG: out1 init = 0.0{{$}}
      // EXE-TGT-USE-STDIO-DAG: out2 init = 0.0{{$}}
      // EXE-TGT-USE-STDIO-DAG: out2 init = 0.0{{$}}
      TGT_PRINTF("out0 init = %.1f\n", out0);
      TGT_PRINTF("out1 init = %.1f\n", out1);
      TGT_PRINTF("out2 init = %.1f\n", out2);
      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCWorkerClause
      // DMP-NEXT:   ACCVectorClause
      // DMP-NEXT:   ACCReductionClause {{.*}} '+'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out0' 'double'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out1' 'double'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out2' 'double'
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
      // DMP-NEXT:   impl: OMPDistributeParallelForSimdDirective
      // DMP-NEXT:     OMPReductionClause
      // DMP-NEXT:       DeclRefExpr {{.*}} 'out0' 'double'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'out1' 'double'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'out2' 'double'
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop worker vector reduction(+: out0,out1,out2){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for simd reduction(+: out0,out1,out2){{$}}
      // PRT-O-NEXT:  {{^ *}}#pragma omp distribute parallel for simd reduction(+: out0,out1,out2){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker vector reduction(+: out0,out1,out2){{$}}
      #pragma acc loop worker vector reduction(+: out0,out1,out2)
      // PRT-NEXT: for ({{.*}}) {
      for (int i = 0; i < 4; ++i) {
        // DMP: CompoundAssignOperator {{.*}} '+='
        // DMP: CompoundAssignOperator {{.*}} '+='
        // DMP: CompoundAssignOperator {{.*}} '+='
        // PRT-NEXT: out0 +=
        // PRT-NEXT: out1 +=
        // PRT-NEXT: out2 +=
        out0 += -1.1;
        out1 += -1.1;
        out2 += -1.1;
      } // PRT-NEXT: }
    } // PRT-NEXT: }
    // DMP: CallExpr
    // DMP: CallExpr
    // DMP: CallExpr
    // PRT-NEXT: printf
    // PRT-NEXT: printf
    // PRT-NEXT: printf
    // EXE-HOST-NEXT: out0 = -4.1{{$}}
    // EXE-HOST-NEXT: out1 = -3.0{{$}}
    // EXE-HOST-NEXT: out2 = -1.9{{$}}
    //  EXE-OFF-NEXT: out0 = -4.1{{$}}
    //  EXE-OFF-NEXT: out1 = 1.4{{$}}
    //  EXE-OFF-NEXT: out2 = {{.*}}
    printf("out0 = %.1f\n", out0);
    printf("out1 = %.1f\n", out1);
    printf("out2 = %.1f\n", out2);
  } // PRT-NEXT: }

  // Reduction var is not private at loop due to copy clause implied by
  // reduction on combined construct.

  // PRT-NEXT: {
  {
    // PRT-NEXT: float out = -5;
    float out = -5;
    // DMP:      ACCParallelLoopDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCWorkerClause
    // DMP-NEXT:   ACCVectorClause
    // DMP-NEXT:   ACCReductionClause {{.*}} '*'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'float'
    // DMP-NEXT:   ACCParallelDirective
    // DMP-NEXT:     ACCNumGangsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     ACCCopyClause {{.*}} <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'float'
    // DMP-NEXT:     ACCReductionClause {{.*}} '*'
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'float'
    // DMP-NEXT:     impl: OMPTargetTeamsDirective
    // DMP-NEXT:       OMPNum_teamsClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:       OMPMapClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out' 'float'
    // DMP-NEXT:       OMPReductionClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out' 'float'
    // DMP:          ACCLoopDirective
    // DMP-NEXT:       ACCWorkerClause
    // DMP-NEXT:       ACCVectorClause
    // DMP-NEXT:       ACCReductionClause {{.*}} '*'
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out' 'float'
    // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:       ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:       impl: OMPDistributeParallelForSimdDirective
    // DMP-NEXT:         OMPReductionClause
    // DMP-NEXT:           DeclRefExpr {{.*}} 'out' 'float'
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel loop num_gangs(2) worker vector reduction(*: out){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) map(ompx_hold,tofrom: out) reduction(*: out){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for simd reduction(*: out){{$}}
    //
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) map(ompx_hold,tofrom: out) reduction(*: out){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp distribute parallel for simd reduction(*: out){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) worker vector reduction(*: out){{$}}
    #pragma acc parallel loop num_gangs(2) worker vector reduction(*: out)
    // PRT-NEXT: for ({{.*}}) {
    for (int i = 0; i < 4; ++i) {
      // DMP: CompoundAssignOperator {{.*}} '*='
      // PRT-NEXT: out *= 3;
      out *= 3;
    } // PRT-NEXT: }
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: out = -405.0{{$}}
    printf("out = %.1f\n", out);
  } // PRT-NEXT: }

  //----------------------------------------------------------------------------
  // gang and explicit acc parallel reduction.
  //
  // Notice that the reduction clause for the gang-local variable "in" is
  // completely discarded during translation to OpenMP because gang reductions
  // cannot affect gang-local variables.  Thus, each gang operates on its own
  // copy of "in" as if the reduction were not specified.
  //
  // It's impossible to have an acc parallel loop version of this because the
  // reduction then applies to the acc loop not the acc parallel.  Thus, that
  // version is tested in the next section instead.
  //----------------------------------------------------------------------------

  // DMP-LABEL: StringLiteral {{.*}} "gang and explicit acc parallel reduction\n"
  // PRT-LABEL: printf("gang and explicit acc parallel reduction\n");
  //   EXE-NOT: {{.}}
  // EXE-LABEL: gang and explicit acc parallel reduction{{$}}
  //   EXE-NOT: {{.}}
  printf("gang and explicit acc parallel reduction\n");

  // PRT-NEXT: {
  {
    // PRT-NEXT: double out =
    double out = 0.3;
    // DMP:      ACCParallelDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCReductionClause {{.*}} '+'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'double'
    // DMP-NEXT:   ACCCopyClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'double'
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'double'
    // DMP-NEXT:     OMPMapClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'double'
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(2) reduction(+: out){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) reduction(+: out) map(ompx_hold,tofrom: out){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) reduction(+: out) map(ompx_hold,tofrom: out){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2) reduction(+: out){{$}}
    #pragma acc parallel num_gangs(2) reduction(+: out)
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      // PRT-NEXT: int in = 3;
      int in = 3;
      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCGangClause
      // DMP-NEXT:   ACCReductionClause {{.*}} '+'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'double'
      // DMP-NEXT:   ACCReductionClause {{.*}} '*'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'in' 'int'
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   impl: OMPDistributeDirective
      // DMP-NOT:      OMPReductionClause
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop gang reduction(+: out) reduction(*: in){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
      // PRT-O-NEXT:  {{^ *}}#pragma omp distribute{{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang reduction(+: out) reduction(*: in){{$}}
      #pragma acc loop gang reduction(+: out) reduction(*: in)
      // PRT-NEXT: for ({{.*}}) {
      for (int i = 0; i < 4; ++i) {
        // DMP: CompoundAssignOperator {{.*}} '+='
        // PRT-NEXT: out +=
        out += -1.1;
        // DMP: CompoundAssignOperator {{.*}} '*='
        // PRT-NEXT: in *=
        in *= 2;
      } // PRT-NEXT: }
      // DMP: CallExpr
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // EXE-TGT-USE-STDIO-NEXT: in = 12{{$}}
      // EXE-TGT-USE-STDIO-NEXT: in = 12{{$}}
      TGT_PRINTF("in = %d\n", in);
    } // PRT-NEXT: }
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: out = -4.1
    printf("out = %.1f\n", out);
  } // PRT-NEXT: }

  //----------------------------------------------------------------------------
  // gang and implicit acc parallel reduction.
  //----------------------------------------------------------------------------

  // DMP-LABEL: StringLiteral {{.*}} "gang and implicit acc parallel reduction\n"
  // PRT-LABEL: printf("gang and implicit acc parallel reduction\n");
  //   EXE-NOT: {{.}}
  // EXE-LABEL: gang and implicit acc parallel reduction{{$}}
  //   EXE-NOT: {{.}}
  printf("gang and implicit acc parallel reduction\n");

  // PRT-NEXT: {
  {
    // PRT-NEXT: double out =
    double out = 0.3;
    // DMP:      ACCParallelDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCCopyClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'double'
    // DMP-NEXT:   ACCReductionClause {{.*}} <implicit> '+'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'double'
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     OMPMapClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'double'
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'double'
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(2) copy(out){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) map(ompx_hold,tofrom: out) reduction(+: out){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) map(ompx_hold,tofrom: out) reduction(+: out){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2) copy(out){{$}}
    #pragma acc parallel num_gangs(2) copy(out)
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      // Make sure accessing out before the reduction doesn't produce a
      // conflicting implicit firstprivate clause.
      // DMP: CallExpr
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // EXE-TGT-USE-STDIO-NEXT: out init = 0.0{{$}}
      // EXE-TGT-USE-STDIO-NEXT: out init = 0.0{{$}}
      TGT_PRINTF("out init = %.1f\n", out);
      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCGangClause
      // DMP-NEXT:   ACCReductionClause {{.*}} '+'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'double'
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   impl: OMPDistributeDirective
      // DMP-NOT:      OMPReductionClause
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop gang reduction(+: out){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
      // PRT-O-NEXT:  {{^ *}}#pragma omp distribute{{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang reduction(+: out){{$}}
      #pragma acc loop gang reduction(+: out)
      // PRT-NEXT: for ({{.*}}) {
      for (int i = 0; i < 4; ++i) {
        // DMP: CompoundAssignOperator {{.*}} '+='
        // PRT-NEXT: out +=
        out += -1.1;
      } // PRT-NEXT: }
    } // PRT-NEXT: }
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: out = -4.1
    printf("out = %.1f\n", out);
  } // PRT-NEXT: }

  // PRT-NEXT: {
  {
    // PRT-NEXT: double out =
    double out = 0.3;
    // DMP:      ACCParallelLoopDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCGangClause
    // DMP-NEXT:   ACCReductionClause {{.*}} '+'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'double'
    // DMP-NEXT:   effect: ACCParallelDirective
    // DMP-NEXT:     ACCNumGangsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     ACCCopyClause {{.*}} <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'double'
    // DMP-NEXT:     ACCReductionClause {{.*}} <implicit> '+'
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'double'
    // DMP-NEXT:     impl: OMPTargetTeamsDirective
    // DMP-NEXT:       OMPNum_teamsClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:       OMPMapClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out' 'double'
    // DMP-NEXT:       OMPReductionClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out' 'double'
    // DMP:          ACCLoopDirective
    // DMP-NEXT:       ACCGangClause
    // DMP-NEXT:       ACCReductionClause {{.*}} '+'
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out' 'double'
    // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:       impl: OMPDistributeDirective
    // DMP-NOT:          OMPReductionClause
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel loop num_gangs(2) gang reduction(+: out){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) map(ompx_hold,tofrom: out) reduction(+: out){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
    //
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) map(ompx_hold,tofrom: out) reduction(+: out){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp distribute{{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) gang reduction(+: out){{$}}
    #pragma acc parallel loop num_gangs(2) gang reduction(+: out)
    // PRT-NEXT: for ({{.*}}) {
    for (int i = 0; i < 4; ++i) {
      // DMP: CompoundAssignOperator {{.*}} '+='
      // PRT-NEXT: out +=
      out += -1.1;
    } // PRT-NEXT: }
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: out = -4.1
    printf("out = %.1f\n", out);
  } // PRT-NEXT: }

  //----------------------------------------------------------------------------
  // gang, worker, and explicit acc parallel reduction.
  //----------------------------------------------------------------------------

  // DMP-LABEL: StringLiteral {{.*}} "gang, worker, and explicit acc parallel reduction\n"
  // PRT-LABEL: printf("gang, worker, and explicit acc parallel reduction\n");
  //   EXE-NOT: {{.}}
  // EXE-LABEL: gang, worker, and explicit acc parallel reduction{{$}}
  //   EXE-NOT: {{.}}
  printf("gang, worker, and explicit acc parallel reduction\n");

  // PRT-NEXT: {
  {
    // PRT-NEXT: int out = 3;
    int out = 3;
    // DMP:      ACCParallelDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCReductionClause {{.*}} '+'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:   ACCCopyClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:     OMPMapClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'int'
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(2) reduction(+: out){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) reduction(+: out) map(ompx_hold,tofrom: out){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) reduction(+: out) map(ompx_hold,tofrom: out){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2) reduction(+: out){{$}}
    #pragma acc parallel num_gangs(2) reduction(+: out)
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      // PRT-NEXT: int in = 3;
      int in = 3;
      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCGangClause
      // DMP-NEXT:   ACCWorkerClause
      // DMP-NEXT:   ACCReductionClause {{.*}} '+'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'in' 'int'
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   impl: OMPDistributeParallelForDirective
      // DMP-NEXT:     OMPReductionClause
      // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'int'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'in' 'int'
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop gang worker reduction(+: out,in){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for reduction(+: out,in){{$}}
      // PRT-O-NEXT:  {{^ *}}#pragma omp distribute parallel for reduction(+: out,in){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang worker reduction(+: out,in){{$}}
      #pragma acc loop gang worker reduction(+: out,in)
      // PRT-NEXT: for ({{.*}}) {
      for (int i = 0; i < 4; ++i) {
        // DMP: CompoundAssignOperator {{.*}} '+='
        // PRT-NEXT: out +=
        out += 2;
        // DMP: CompoundAssignOperator {{.*}} '+='
        // PRT-NEXT: in +=
        in += 2;
      } // PRT-NEXT: }
      // DMP: CallExpr
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // EXE-TGT-USE-STDIO-NEXT: in = 7{{$}}
      // EXE-TGT-USE-STDIO-NEXT: in = 7{{$}}
      TGT_PRINTF("in = %d\n", in);
    } // PRT-NEXT: }
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: out = 11{{$}}
    printf("out = %d\n", out);
  } // PRT-NEXT: }

  // PRT-NEXT: {
  {
    // PRT-NEXT: int out = 3;
    int out = 3;
    // DMP:      ACCParallelLoopDirective
    // DMP-NEXT:   ACCGangClause
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCWorkerClause
    // DMP-NEXT:   ACCReductionClause {{.*}} '+'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:   effect: ACCParallelDirective
    // DMP-NEXT:     ACCNumGangsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     ACCCopyClause {{.*}} <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:     ACCReductionClause {{.*}} <implicit> '+'
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:     impl: OMPTargetTeamsDirective
    // DMP-NEXT:       OMPNum_teamsClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:       OMPMapClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:       OMPReductionClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out' 'int'
    // DMP:          ACCLoopDirective
    // DMP-NEXT:       ACCGangClause
    // DMP-NEXT:       ACCWorkerClause
    // DMP-NEXT:       ACCReductionClause {{.*}} '+'
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:       impl: OMPDistributeParallelForDirective
    // DMP-NEXT:         OMPReductionClause
    // DMP-NEXT:           DeclRefExpr {{.*}} 'out' 'int'
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel loop gang num_gangs(2) worker reduction(+: out){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) map(ompx_hold,tofrom: out) reduction(+: out){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for reduction(+: out){{$}}
    //
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) map(ompx_hold,tofrom: out) reduction(+: out){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp distribute parallel for reduction(+: out){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop gang num_gangs(2) worker reduction(+: out){{$}}
    #pragma acc parallel loop gang num_gangs(2) worker reduction(+: out)
    // PRT-NEXT: for ({{.*}}) {
    for (int i = 0; i < 4; ++i) {
      // DMP: CompoundAssignOperator {{.*}} '+='
      // PRT-NEXT: out +=
      out += 2;
    } // PRT-NEXT: }
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: out = 11{{$}}
    printf("out = %d\n", out);
  } // PRT-NEXT: }

  //----------------------------------------------------------------------------
  // gang, vector, and implicit acc parallel reduction.
  //
  // For acc parallel loop, we extend this test too, not to distinguish
  // implicit from the explicit version in the previous test, but to
  // distinguish vector from the worker version in the previous test.
  //----------------------------------------------------------------------------

  // DMP-LABEL: StringLiteral {{.*}} "gang, vector, and implicit acc parallel reduction\n"
  // PRT-LABEL: printf("gang, vector, and implicit acc parallel reduction\n");
  //   EXE-NOT: {{.}}
  // EXE-LABEL: gang, vector, and implicit acc parallel reduction{{$}}
  //   EXE-NOT: {{.}}
  printf("gang, vector, and implicit acc parallel reduction\n");

  // PRT-NEXT: {
  {
    // PRT-NEXT: int out = 3;
    int out = 3;
    // DMP:      ACCParallelDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCCopyClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:   ACCReductionClause {{.*}} <implicit> '+'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     OMPMapClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'int'
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(2) copy(out){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) map(ompx_hold,tofrom: out) reduction(+: out){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) map(ompx_hold,tofrom: out) reduction(+: out){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2) copy(out){{$}}
    #pragma acc parallel num_gangs(2) copy(out)
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      // PRT-NEXT: int in = 3;
      int in = 3;
      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCGangClause
      // DMP-NEXT:   ACCVectorClause
      // DMP-NEXT:   ACCReductionClause {{.*}} '+'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'in' 'int'
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   impl: OMPDistributeSimdDirective
      // DMP-NEXT:     OMPReductionClause
      // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'int'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'in' 'int'
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop gang vector reduction(+: out,in){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute simd reduction(+: out,in){{$}}
      // PRT-O-NEXT:  {{^ *}}#pragma omp distribute simd reduction(+: out,in){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang vector reduction(+: out,in){{$}}
      #pragma acc loop gang vector reduction(+: out,in)
      // PRT-NEXT: for ({{.*}}) {
      for (int i = 0; i < 4; ++i) {
        // DMP: CompoundAssignOperator {{.*}} '+='
        // PRT-NEXT: out +=
        out += 2;
        // DMP: CompoundAssignOperator {{.*}} '+='
        // PRT-NEXT: in +=
        in += 2;
      } // PRT-NEXT: }
      // DMP: CallExpr
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // EXE-TGT-USE-STDIO-NEXT: in = 7{{$}}
      // EXE-TGT-USE-STDIO-NEXT: in = 7{{$}}
      TGT_PRINTF("in = %d\n", in);
    } // PRT-NEXT: }
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: out = 11
    printf("out = %d\n", out);
  } // PRT-NEXT: }

  // PRT-NEXT: {
  {
    // PRT-NEXT: int out = 3;
    int out = 3;
    // DMP:      ACCParallelLoopDirective
    // DMP-NEXT:   ACCVectorClause
    // DMP-NEXT:   ACCGangClause
    // DMP-NEXT:   ACCReductionClause {{.*}} '+'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   effect: ACCParallelDirective
    // DMP-NEXT:     ACCNumGangsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     ACCCopyClause {{.*}} <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:     ACCReductionClause {{.*}} <implicit> '+'
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:     impl: OMPTargetTeamsDirective
    // DMP-NEXT:       OMPNum_teamsClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:       OMPMapClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:       OMPReductionClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out' 'int'
    // DMP:          ACCLoopDirective
    // DMP-NEXT:       ACCVectorClause
    // DMP-NEXT:       ACCGangClause
    // DMP-NEXT:       ACCReductionClause {{.*}} '+'
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:       impl: OMPDistributeSimdDirective
    // DMP-NEXT:         OMPReductionClause
    // DMP-NEXT:           DeclRefExpr {{.*}} 'out' 'int'
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel loop vector gang reduction(+: out) num_gangs(2){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) map(ompx_hold,tofrom: out) reduction(+: out){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute simd reduction(+: out){{$}}
    //
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) map(ompx_hold,tofrom: out) reduction(+: out){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp distribute simd reduction(+: out){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop vector gang reduction(+: out) num_gangs(2){{$}}
    #pragma acc parallel loop vector gang reduction(+: out) num_gangs(2)
    // PRT-NEXT: for ({{.*}}) {
    for (int i = 0; i < 4; ++i) {
      // DMP: CompoundAssignOperator {{.*}} '+='
      // PRT-NEXT: out +=
      out += 2;
    } // PRT-NEXT: }
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: out = 11
    printf("out = %d\n", out);
  } // PRT-NEXT: }

  //----------------------------------------------------------------------------
  // gang, worker, vector, and single loop.
  //----------------------------------------------------------------------------

  // DMP-LABEL: StringLiteral {{.*}} "gang, worker, vector, and single loop\n"
  // PRT-LABEL: printf("gang, worker, vector, and single loop\n");
  //   EXE-NOT: {{.}}
  // EXE-LABEL: gang, worker, vector, and single loop{{$}}
  //   EXE-NOT: {{.}}
  printf("gang, worker, vector, and single loop\n");

  // PRT-NEXT: {
  {
    // PRT-NEXT: int out = 3;
    int out = 3;
    // DMP:      ACCParallelDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCCopyClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:   ACCReductionClause {{.*}} <implicit> '+'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     OMPMapClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'int'
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(2) copy(out){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) map(ompx_hold,tofrom: out) reduction(+: out){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) map(ompx_hold,tofrom: out) reduction(+: out){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2) copy(out){{$}}
    #pragma acc parallel num_gangs(2) copy(out)
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      // PRT-NEXT: int in = 3;
      int in = 3;
      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCGangClause
      // DMP-NEXT:   ACCWorkerClause
      // DMP-NEXT:   ACCVectorClause
      // DMP-NEXT:   ACCReductionClause {{.*}} '+'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'in' 'int'
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   impl: OMPDistributeParallelForSimdDirective
      // DMP-NEXT:     OMPReductionClause
      // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'int'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'in' 'int'
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop gang worker vector reduction(+: out,in){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for simd reduction(+: out,in){{$}}
      // PRT-O-NEXT:  {{^ *}}#pragma omp distribute parallel for simd reduction(+: out,in){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang worker vector reduction(+: out,in){{$}}
      #pragma acc loop gang worker vector reduction(+: out,in)
      // PRT-NEXT: for ({{.*}}) {
      for (int i = 0; i < 4; ++i) {
        // DMP: CompoundAssignOperator {{.*}} '+='
        // PRT-NEXT: out +=
        out += 2;
        // DMP: CompoundAssignOperator {{.*}} '+='
        // PRT-NEXT: in +=
        in += 2;
      } // PRT-NEXT: }
      // DMP: CallExpr
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // EXE-TGT-USE-STDIO-NEXT: in = 7{{$}}
      // EXE-TGT-USE-STDIO-NEXT: in = 7{{$}}
      TGT_PRINTF("in = %d\n", in);
    } // PRT-NEXT: }
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: out = 11{{$}}
    printf("out = %d\n", out);
  } // PRT-NEXT: }

  // PRT-NEXT: {
  {
    // PRT-NEXT: int out = 3;
    int out = 3;
    // DMP:      ACCParallelLoopDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCGangClause
    // DMP-NEXT:   ACCWorkerClause
    // DMP-NEXT:   ACCVectorClause
    // DMP-NEXT:   ACCReductionClause {{.*}} '+'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:   effect: ACCParallelDirective
    // DMP-NEXT:     ACCNumGangsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     ACCCopyClause {{.*}} <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:     ACCReductionClause {{.*}} <implicit> '+'
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:     impl: OMPTargetTeamsDirective
    // DMP-NEXT:       OMPNum_teamsClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:       OMPMapClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:       OMPReductionClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out' 'int'
    // DMP:          ACCLoopDirective
    // DMP-NEXT:       ACCGangClause
    // DMP-NEXT:       ACCWorkerClause
    // DMP-NEXT:       ACCVectorClause
    // DMP-NEXT:       ACCReductionClause {{.*}} '+'
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:       impl: OMPDistributeParallelForSimdDirective
    // DMP-NEXT:         OMPReductionClause
    // DMP-NEXT:           DeclRefExpr {{.*}} 'out' 'int'
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel loop num_gangs(2) gang worker vector reduction(+: out){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) map(ompx_hold,tofrom: out) reduction(+: out){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for simd reduction(+: out){{$}}
    //
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) map(ompx_hold,tofrom: out) reduction(+: out){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp distribute parallel for simd reduction(+: out){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) gang worker vector reduction(+: out){{$}}
    #pragma acc parallel loop num_gangs(2) gang worker vector reduction(+: out)
    // PRT-NEXT: for ({{.*}}) {
    for (int i = 0; i < 4; ++i) {
      // DMP: CompoundAssignOperator {{.*}} '+='
      // PRT-NEXT: out +=
      out += 2;
    } // PRT-NEXT: }
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: out = 11{{$}}
    printf("out = %d\n", out);
  } // PRT-NEXT: }

  //----------------------------------------------------------------------------
  // gang, worker, vector, and separate nested loops.
  //----------------------------------------------------------------------------

  // DMP-LABEL: StringLiteral {{.*}} "gang, worker, vector, and separate nested loops\n"
  // PRT-LABEL: printf("gang, worker, vector, and separate nested loops\n");
  //   EXE-NOT: {{.}}
  // EXE-LABEL: gang, worker, vector, and separate nested loops{{$}}
  //   EXE-NOT: {{.}}
  printf("gang, worker, vector, and separate nested loops\n");

  // PRT-NEXT: {
  {
    // PRT-NEXT: int out = 3;
    int out = 3;
    // DMP:      ACCParallelDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCCopyClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:   ACCReductionClause {{.*}} <implicit> '+'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     OMPMapClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'int'
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(2) copy(out){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) map(ompx_hold,tofrom: out) reduction(+: out){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) map(ompx_hold,tofrom: out) reduction(+: out){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2) copy(out){{$}}
    #pragma acc parallel num_gangs(2) copy(out)
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      // PRT-NEXT: int in = 3;
      int in = 3;
      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCGangClause
      // DMP-NEXT:   ACCReductionClause {{.*}} '+'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'in' 'int'
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   impl: OMPDistributeDirective
      // DMP-NOT:      OMPReductionClause
      // DMP:          Stmt
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop gang reduction(+: out,in){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
      // PRT-O-NEXT:  {{^ *}}#pragma omp distribute{{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang reduction(+: out,in){{$}}
      #pragma acc loop gang reduction(+: out,in)
      // PRT-NEXT: for ({{.*}}) {
      for (int i = 0; i < 2; ++i) {
        // DMP:      ACCLoopDirective
        // DMP-NEXT:   ACCWorkerClause
        // DMP-NEXT:   ACCReductionClause {{.*}} '+'
        // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
        // DMP-NEXT:     DeclRefExpr {{.*}} 'in' 'int'
        // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
        // DMP-NEXT:   impl: OMPParallelForDirective
        // DMP-NEXT:     OMPReductionClause
        // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'int'
        // DMP-NEXT:       DeclRefExpr {{.*}} 'in' 'int'
        //
        // PRT-A-NEXT:  {{^ *}}#pragma acc loop worker reduction(+: out,in){{$}}
        // PRT-AO-NEXT: {{^ *}}// #pragma omp parallel for reduction(+: out,in){{$}}
        // PRT-O-NEXT:  {{^ *}}#pragma omp parallel for reduction(+: out,in){{$}}
        // PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker reduction(+: out,in){{$}}
        #pragma acc loop worker reduction(+: out,in)
        // PRT-NEXT: for ({{.*}}) {
        for (int j = 0; j < 2; ++j) {
          // DMP:      ACCLoopDirective
          // DMP-NEXT:   ACCVectorClause
          // DMP-NEXT:   ACCReductionClause {{.*}} '+'
          // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
          // DMP-NEXT:     DeclRefExpr {{.*}} 'in' 'int'
          // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
          // DMP-NEXT:   impl: OMPSimdDirective
          // DMP-NEXT:     OMPReductionClause
          // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'int'
          // DMP-NEXT:       DeclRefExpr {{.*}} 'in' 'int'
          //
          // PRT-A-NEXT:  {{^ *}}#pragma acc loop vector reduction(+: out,in){{$}}
          // PRT-AO-NEXT: {{^ *}}// #pragma omp simd reduction(+: out,in){{$}}
          // PRT-O-NEXT:  {{^ *}}#pragma omp simd reduction(+: out,in){{$}}
          // PRT-OA-NEXT: {{^ *}}// #pragma acc loop vector reduction(+: out,in){{$}}
          #pragma acc loop vector reduction(+: out,in)
          // PRT-NEXT: for ({{.*}}) {
          for (int k = 0; k < 2; ++k) {
            // DMP: CompoundAssignOperator {{.*}} '+='
            // PRT-NEXT: out +=
            out += 2;
            // DMP: CompoundAssignOperator {{.*}} '+='
            // PRT-NEXT: in +=
            in += 2;
          } // PRT-NEXT: }
        } // PRT-NEXT: }
      } // PRT-NEXT: }
      // DMP: CallExpr
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // EXE-TGT-USE-STDIO-NEXT: in = 11{{$}}
      // EXE-TGT-USE-STDIO-NEXT: in = 11{{$}}
      TGT_PRINTF("in = %d\n", in);
    } // PRT-NEXT: }
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: out = 19{{$}}
    printf("out = %d\n", out);
  } // PRT-NEXT: }

  // PRT-NEXT: {
  {
    // PRT-NEXT: int out = 3;
    int out = 3;
    // DMP:      ACCParallelLoopDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCGangClause
    // DMP-NEXT:   ACCReductionClause {{.*}} '+'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:   effect: ACCParallelDirective
    // DMP-NEXT:     ACCNumGangsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     ACCCopyClause {{.*}} <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:     ACCReductionClause {{.*}} <implicit> '+'
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:     impl: OMPTargetTeamsDirective
    // DMP-NEXT:       OMPNum_teamsClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:       OMPMapClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:       OMPReductionClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out' 'int'
    // DMP:          ACCLoopDirective
    // DMP-NEXT:       ACCGangClause
    // DMP-NEXT:       ACCReductionClause {{.*}} '+'
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:       impl: OMPDistributeDirective
    // DMP-NOT:          OMPReductionClause
    // DMP-NEXT:         Stmt
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel loop num_gangs(2) gang reduction(+: out){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) map(ompx_hold,tofrom: out) reduction(+: out){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
    //
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) map(ompx_hold,tofrom: out) reduction(+: out){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp distribute{{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) gang reduction(+: out){{$}}
    #pragma acc parallel loop num_gangs(2) gang reduction(+: out)
    // PRT-NEXT: for ({{.*}}) {
    for (int i = 0; i < 2; ++i) {
      // PRT-NEXT: int in = 3;
      int in = 3;
      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCWorkerClause
      // DMP-NEXT:   ACCReductionClause {{.*}} '+'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'in' 'int'
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   impl: OMPParallelForDirective
      // DMP-NEXT:     OMPReductionClause
      // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'int'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'in' 'int'
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop worker reduction(+: out,in){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp parallel for reduction(+: out,in){{$}}
      //
      // PRT-O-NEXT:  {{^ *}}#pragma omp parallel for reduction(+: out,in){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker reduction(+: out,in){{$}}
      #pragma acc loop worker reduction(+: out,in)
      // PRT-NEXT: for ({{.*}}) {
      for (int j = 0; j < 2; ++j) {
        // DMP:      ACCLoopDirective
        // DMP-NEXT:   ACCVectorClause
        // DMP-NEXT:   ACCReductionClause {{.*}} '+'
        // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
        // DMP-NEXT:     DeclRefExpr {{.*}} 'in' 'int'
        // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
        // DMP-NEXT:   impl: OMPSimdDirective
        // DMP-NEXT:     OMPReductionClause
        // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'int'
        // DMP-NEXT:       DeclRefExpr {{.*}} 'in' 'int'
        //
        // PRT-A-NEXT:  {{^ *}}#pragma acc loop vector reduction(+: out,in){{$}}
        // PRT-AO-NEXT: {{^ *}}// #pragma omp simd reduction(+: out,in){{$}}
        //
        // PRT-O-NEXT:  {{^ *}}#pragma omp simd reduction(+: out,in){{$}}
        // PRT-OA-NEXT: {{^ *}}// #pragma acc loop vector reduction(+: out,in){{$}}
        #pragma acc loop vector reduction(+: out,in)
        // PRT-NEXT: for ({{.*}}) {
        for (int k = 0; k < 2; ++k) {
          // DMP: CompoundAssignOperator {{.*}} '+='
          // PRT-NEXT: out +=
          out += 2;
          // DMP: CompoundAssignOperator {{.*}} '+='
          // PRT-NEXT: in +=
          in += 2;
        } // PRT-NEXT: }
      } // PRT-NEXT: }
    } // PRT-NEXT: }
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: out = 19{{$}}
    printf("out = %d\n", out);
  } // PRT-NEXT: }

  //----------------------------------------------------------------------------
  // gang reductions from sibling loops.
  //----------------------------------------------------------------------------

  // DMP-LABEL: StringLiteral {{.*}} "gang reductions from sibling loops\n"
  // PRT-LABEL: printf("gang reductions from sibling loops\n");
  //   EXE-NOT: {{.}}
  // EXE-LABEL: gang reductions from sibling loops{{$}}
  //   EXE-NOT: {{.}}
  printf("gang reductions from sibling loops\n");

  // PRT-NEXT: {
  {
    // PRT-NEXT: int out = 5;
    int out = 5;
    // DMP:      ACCParallelDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCCopyClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:   ACCReductionClause {{.*}} <implicit> '+'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     OMPMapClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'int'
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(2) copy(out){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) map(ompx_hold,tofrom: out) reduction(+: out){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) map(ompx_hold,tofrom: out) reduction(+: out){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2) copy(out){{$}}
    #pragma acc parallel num_gangs(2) copy(out)
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCGangClause
      // DMP-NEXT:   ACCReductionClause {{.*}} '+'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   impl: OMPDistributeDirective
      // DMP-NOT:      OMPReductionClause
      // DMP:          Stmt
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop gang reduction(+: out){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
      // PRT-O-NEXT:  {{^ *}}#pragma omp distribute{{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang reduction(+: out){{$}}
      #pragma acc loop gang reduction(+: out)
      // PRT-NEXT: for ({{.*}}) {
      for (int i = 0; i < 2; ++i) {
        // DMP: CompoundAssignOperator {{.*}} '+='
        // PRT-NEXT: out += 2;
        out += 2;
      } // PRT-NEXT: }
      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCGangClause
      // DMP-NEXT:   ACCReductionClause {{.*}} '+'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   impl: OMPDistributeDirective
      // DMP-NOT:      OMPReductionClause
      // DMP:          Stmt
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop gang reduction(+: out){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
      // PRT-O-NEXT:  {{^ *}}#pragma omp distribute{{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang reduction(+: out){{$}}
      #pragma acc loop gang reduction(+: out)
      // PRT-NEXT: for ({{.*}}) {
      for (int i = 0; i < 2; ++i) {
        // DMP: CompoundAssignOperator {{.*}} '+='
        // PRT-NEXT: out += 3;
        out += 3;
      } // PRT-NEXT: }
    } // PRT-NEXT: }
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: out = 15
    printf("out = %d\n", out);
  } // PRT-NEXT: }

  //----------------------------------------------------------------------------
  // gang reductions, shadowing local variable out of scope.
  //----------------------------------------------------------------------------

  // DMP-LABEL: StringLiteral {{.*}} "gang reductions, shadowing local variable out of scope\n"
  // PRT-LABEL: printf("gang reductions, shadowing local variable out of scope\n");
  //   EXE-NOT: {{.}}
  // EXE-LABEL: gang reductions, shadowing local variable out of scope{{$}}
  //   EXE-NOT: {{.}}
  printf("gang reductions, shadowing local variable out of scope\n");

  // PRT-NEXT: {
  {
    // PRT-NEXT: int out = 5;
    int out = 5;
    // DMP:      ACCParallelDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCCopyClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:   ACCReductionClause {{.*}} <implicit> '+'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     OMPMapClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'int'
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(2) copy(out){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) map(ompx_hold,tofrom: out) reduction(+: out){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) map(ompx_hold,tofrom: out) reduction(+: out){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2) copy(out){{$}}
    #pragma acc parallel num_gangs(2) copy(out)
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      // PRT-NEXT: {
      {
        // PRT-NEXT: int out = 5;
        int out = 5;
        // DMP:      ACCLoopDirective
        // DMP-NEXT:   ACCGangClause
        // DMP-NEXT:   ACCReductionClause {{.*}} '+'
        // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
        // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
        // DMP-NEXT:   impl: OMPDistributeDirective
        // DMP-NOT:      OMPReductionClause
        // DMP:          Stmt
        //
        // PRT-A-NEXT:  {{^ *}}#pragma acc loop gang reduction(+: out){{$}}
        // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
        // PRT-O-NEXT:  {{^ *}}#pragma omp distribute{{$}}
        // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang reduction(+: out){{$}}
        #pragma acc loop gang reduction(+: out)
        // PRT-NEXT: for ({{.*}}) {
        for (int i = 0; i < 2; ++i) {
          // DMP: CompoundAssignOperator {{.*}} '+='
          // PRT-NEXT: out += 2;
          out += 2;
        } // PRT-NEXT: }
      } // PRT-NEXT: }
      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCGangClause
      // DMP-NEXT:   ACCReductionClause {{.*}} '+'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   impl: OMPDistributeDirective
      // DMP-NOT:      OMPReductionClause
      // DMP:          Stmt
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop gang reduction(+: out){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
      // PRT-O-NEXT:  {{^ *}}#pragma omp distribute{{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang reduction(+: out){{$}}
      #pragma acc loop gang reduction(+: out)
      // PRT-NEXT: for ({{.*}}) {
      for (int i = 0; i < 2; ++i) {
        // DMP: CompoundAssignOperator {{.*}} '+='
        // PRT-NEXT: out += 3;
        out += 3;
      } // PRT-NEXT: }
    } // PRT-NEXT: }
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: out = 11
    printf("out = %d\n", out);
  } // PRT-NEXT: }

  //----------------------------------------------------------------------------
  // gang reduction from within sequential acc loop.
  //----------------------------------------------------------------------------

  // DMP-LABEL: StringLiteral {{.*}} "gang reduction from within sequential acc loop\n"
  // PRT-LABEL: printf("gang reduction from within sequential acc loop\n");
  //   EXE-NOT: {{.}}
  // EXE-LABEL: gang reduction from within sequential acc loop{{$}}
  //   EXE-NOT: {{.}}
  printf("gang reduction from within sequential acc loop\n");

  // PRT-NEXT: {
  {
    // PRT-NEXT: int out = 5;
    int out = 5;
    // DMP:      ACCParallelDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCCopyClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:   ACCReductionClause {{.*}} <implicit> '+'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     OMPMapClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'int'
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(2) copy(out){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) map(ompx_hold,tofrom: out) reduction(+: out){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) map(ompx_hold,tofrom: out) reduction(+: out){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2) copy(out){{$}}
    #pragma acc parallel num_gangs(2) copy(out)
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
      // DMP-NEXT:   impl: ForStmt
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop
      // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
      // PRT-A-SAME:  {{^$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop // discarded in OpenMP translation{{$}}
      #pragma acc loop
      // PRT-NEXT: for ({{.*}}) {
      for (int i0 = 0; i0 < 2; ++i0) {
        // DMP:      ACCLoopDirective
        // DMP-NEXT:   ACCSeqClause
        // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
        // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
        // DMP-NEXT:   impl: ForStmt
        //
        // PRT-A-NEXT:  {{^ *}}#pragma acc loop seq
        // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
        // PRT-A-SAME:  {{^$}}
        // PRT-OA-NEXT: {{^ *}}// #pragma acc loop seq // discarded in OpenMP translation{{$}}
        #pragma acc loop seq
        // PRT-NEXT: for ({{.*}}) {
        for (int i1 = 0; i1 < 2; ++i1) {
          // DMP:      ACCLoopDirective
          // DMP-NEXT:   ACCIndependentClause
          // DMP-NOT:      <implicit>
          // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
          // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
          // DMP-NEXT:   impl: ForStmt
          //
          // PRT-A-NEXT:  {{^ *}}#pragma acc loop independent
          // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
          // PRT-A-SAME:  {{^$}}
          // PRT-OA-NEXT: {{^ *}}// #pragma acc loop independent // discarded in OpenMP translation{{$}}
          #pragma acc loop independent
          // PRT-NEXT: for ({{.*}}) {
          for (int i2 = 0; i2 < 2; ++i2) {
            // DMP:      ACCLoopDirective
            // DMP-NEXT:   ACCAutoClause
            // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
            // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
            // DMP-NEXT:   impl: ForStmt
            //
            // PRT-A-NEXT:  {{^ *}}#pragma acc loop auto
            // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
            // PRT-A-SAME:  {{^$}}
            // PRT-OA-NEXT: {{^ *}}// #pragma acc loop auto // discarded in OpenMP translation{{$}}
            #pragma acc loop auto
            // PRT-NEXT: for ({{.*}}) {
            for (int i3 = 0; i3 < 2; ++i3) {
              // DMP:      ACCLoopDirective
              // DMP-NEXT:   ACCGangClause
              // DMP-NEXT:   ACCReductionClause {{.*}} '+'
              // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
              // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
              // DMP-NEXT:   impl: OMPDistributeDirective
              // DMP-NOT:      OMPReductionClause
              // DMP:          Stmt
              //
              // PRT-A-NEXT:  {{^ *}}#pragma acc loop gang reduction(+: out){{$}}
              // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
              // PRT-O-NEXT:  {{^ *}}#pragma omp distribute{{$}}
              // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang reduction(+: out){{$}}
              #pragma acc loop gang reduction(+: out)
              // PRT-NEXT: for ({{.*}}) {
              for (int j = 0; j < 2; ++j) {
                // DMP: CompoundAssignOperator {{.*}} '+='
                // PRT-NEXT: out += 2;
                out += 2;
              } // PRT-NEXT: }
            } // PRT-NEXT: }
          } // PRT-NEXT: }
        } // PRT-NEXT: }
      } // PRT-NEXT: }
    } // PRT-NEXT: }
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: out = 69
    printf("out = %d\n", out);
  } // PRT-NEXT: }

  // PRT-NEXT: {
  {
    // PRT-NEXT: int out = 5;
    int out = 5;
    // DMP:      ACCParallelLoopDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCCopyClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:   effect: ACCParallelDirective
    // DMP-NEXT:     ACCNumGangsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     ACCCopyClause
    // DMP-NOT:        <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:     ACCReductionClause {{.*}} <implicit> '+'
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:     impl: OMPTargetTeamsDirective
    // DMP-NEXT:       OMPNum_teamsClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:       OMPMapClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:       OMPReductionClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out' 'int'
    // DMP:          ACCLoopDirective
    // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:       ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:       impl: ForStmt
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel loop num_gangs(2) copy(out){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) map(ompx_hold,tofrom: out) reduction(+: out){{$}}
    //
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) map(ompx_hold,tofrom: out) reduction(+: out){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) copy(out){{$}}
    #pragma acc parallel loop num_gangs(2) copy(out)
    // PRT-NEXT: for ({{.*}}) {
    for (int i0 = 0; i0 < 2; ++i0) {
      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCSeqClause
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
      // DMP-NEXT:   impl: ForStmt
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop seq
      // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
      // PRT-A-SAME:  {{^$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop seq // discarded in OpenMP translation{{$}}
      #pragma acc loop seq
      // PRT-NEXT: for ({{.*}}) {
      for (int i1 = 0; i1 < 2; ++i1) {
        // DMP:      ACCLoopDirective
        // DMP-NEXT:   ACCIndependentClause
        // DMP-NOT:      <implicit>
        // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
        // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
        // DMP-NEXT:   impl: ForStmt
        //
        // PRT-A-NEXT:  {{^ *}}#pragma acc loop independent
        // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
        // PRT-A-SAME:  {{^$}}
        // PRT-OA-NEXT: {{^ *}}// #pragma acc loop independent // discarded in OpenMP translation{{$}}
        #pragma acc loop independent
        // PRT-NEXT: for ({{.*}}) {
        for (int i2 = 0; i2 < 2; ++i2) {
          // DMP:      ACCLoopDirective
          // DMP-NEXT:   ACCAutoClause
          // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
          // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
          // DMP-NEXT:   impl: ForStmt
          //
          // PRT-A-NEXT:  {{^ *}}#pragma acc loop auto
          // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
          // PRT-A-SAME:  {{^$}}
          // PRT-OA-NEXT: {{^ *}}// #pragma acc loop auto // discarded in OpenMP translation{{$}}
          #pragma acc loop auto
          // PRT-NEXT: for ({{.*}}) {
          for (int i3 = 0; i3 < 2; ++i3) {
            // DMP:      ACCLoopDirective
            // DMP-NEXT:   ACCGangClause
            // DMP-NEXT:   ACCReductionClause {{.*}} '+'
            // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
            // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
            // DMP-NEXT:   impl: OMPDistributeDirective
            // DMP-NOT:      OMPReductionClause
            //
            // PRT-A-NEXT:  {{^ *}}#pragma acc loop gang reduction(+: out){{$}}
            // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
            // PRT-O-NEXT:  {{^ *}}#pragma omp distribute{{$}}
            // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang reduction(+: out){{$}}
            #pragma acc loop gang reduction(+: out)
            // DMP: ForStmt
            // PRT-NEXT: for ({{.*}}) {
            for (int j = 0; j < 2; ++j) {
              // DMP: CompoundAssignOperator {{.*}} '+='
              // PRT-NEXT: out += 2;
              out += 2;
            } // PRT-NEXT: }
          } // PRT-NEXT: }
        } // PRT-NEXT: }
      } // PRT-NEXT: }
    } // PRT-NEXT: }
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: out = 69
    printf("out = %d\n", out);
  } // PRT-NEXT: }

  //----------------------------------------------------------------------------
  // gang reduction on variable explicitly gang-private at acc parallel.
  //
  // TODO: Once we decide how to deal with private/firstprivate and reduction
  // on the same variable, this might be interesting to extend for acc parallel
  // loop.
  //----------------------------------------------------------------------------

  // DMP-LABEL: StringLiteral {{.*}} "gang reduction on variable explicitly gang-private at acc parallel\n"
  // PRT-LABEL: printf("gang reduction on variable explicitly gang-private at acc parallel\n");
  //   EXE-NOT: {{.}}
  // EXE-LABEL: gang reduction on variable explicitly gang-private at acc parallel{{$}}
  //   EXE-NOT: {{.}}
  printf("gang reduction on variable explicitly gang-private at acc parallel\n");

  // PRT-NEXT: {
  {
    // PRT-NEXT: int out0 = 5;
    // PRT-NEXT: int out1 = 5;
    int out0 = 5;
    int out1 = 5;
    // DMP:      ACCParallelDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out0' 'int'
    // DMP-NEXT:   ACCFirstprivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out1' 'int'
    // DMP-NEXT:   ACCNomapClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out0' 'int'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out1' 'int'
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     OMPPrivateClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out0' 'int'
    // DMP-NEXT:     OMPFirstprivateClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out1' 'int'
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(2) private(out0) firstprivate(out1){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) private(out0) firstprivate(out1){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) private(out0) firstprivate(out1){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2) private(out0) firstprivate(out1){{$}}
    #pragma acc parallel num_gangs(2) private(out0) firstprivate(out1)
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCGangClause
      // DMP-NEXT:   ACCReductionClause {{.*}} '+'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out0' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out1' 'int'
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   impl: OMPDistributeDirective
      // DMP-NOT:      OMPReductionClause
      // DMP:          Stmt
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop gang reduction(+: out0,out1){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
      // PRT-O-NEXT:  {{^ *}}#pragma omp distribute{{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang reduction(+: out0,out1){{$}}
      #pragma acc loop gang reduction(+: out0,out1)
      // PRT-NEXT: for ({{.*}}) {
      for (int i = 0; i < 2; ++i) {
        // DMP: CompoundAssignOperator {{.*}} '+='
        // DMP: CompoundAssignOperator {{.*}} '+='
        // PRT-NEXT: out0 += 2;
        // PRT-NEXT: out1 += 2;
        out0 += 2;
        out1 += 2;
      } // PRT-NEXT: }
    } // PRT-NEXT: }
    // DMP: CallExpr
    // DMP: CallExpr
    // PRT-NEXT: printf
    // PRT-NEXT: printf
    // EXE-NEXT: out0 = 5
    // EXE-NEXT: out1 = 5
    printf("out0 = %d\n", out0);
    printf("out1 = %d\n", out1);
  } // PRT-NEXT: }

  //----------------------------------------------------------------------------
  // Gang reduction on variable explicitly privatized at enclosing acc loop not
  // at an enclosing acc parallel.
  //----------------------------------------------------------------------------

  // DMP-LABEL: StringLiteral {{.*}} "gang reduction on variable explicitly privatized at enclosing acc loop\n"
  // PRT-LABEL: printf("gang reduction on variable explicitly privatized at enclosing acc loop\n");
  //   EXE-NOT: {{.}}
  // EXE-LABEL: gang reduction on variable explicitly privatized at enclosing acc loop{{$}}
  //   EXE-NOT: {{.}}
  printf("gang reduction on variable explicitly privatized at enclosing acc loop\n");

  // Each of out0, out1, and out2 is privatized at an acc loop enclosing the
  // acc loop that would otherwise create a gang reduction for it.  In the
  // case of out0 and out1, the latter loop is a gang loop.  In the case of
  // out2, it's a worker loop where out2 would otherwise be gang-shared due to
  // its copy clause on the acc parallel.
  //
  // Because out0 and out2 are in private clauses but out1 is in a reduction
  // clause on an outer acc loop, the acc parallel doesn't need a data clause
  // for out0 or out2, but it does need out1 in a firstprivate in order to
  // access its original value.

  // PRT-NEXT: {
  {
    // PRT-NEXT: int out0 = 5;
    // PRT-NEXT: int out1 = 6;
    // PRT-NEXT: int out2 = 7;
    int out0 = 5;
    int out1 = 6;
    int out2 = 7;
    // DMP:      ACCParallelDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCCopyClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out2' 'int'
    // DMP-NEXT:   ACCFirstprivateClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out1' 'int'
    // DMP-NEXT:   ACCNomapClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out1' 'int'
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     OMPMapClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out2' 'int'
    // DMP-NEXT:     OMPFirstprivateClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out1' 'int'
    // DMP-NOT:      OMP
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(2) present_or_copy(out2) firstprivate(out1){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) map(ompx_hold,tofrom: out2) firstprivate(out1){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) map(ompx_hold,tofrom: out2) firstprivate(out1){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2) present_or_copy(out2) firstprivate(out1){{$}}
    #pragma acc parallel num_gangs(2) present_or_copy(out2) firstprivate(out1)
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCSeqClause
      // DMP-NEXT:   ACCPrivateClause
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out0' 'int'
      // DMP-NEXT:   ACCReductionClause
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out1' 'int'
      // DMP-NEXT:   impl: CompoundStmt
      // DMP-NEXT:     DeclStmt
      // DMP-NEXT:       VarDecl {{.*}} out0 'int'
      // DMP-NEXT:     ForStmt
      // DMP:        ACCLoopDirective
      // DMP-NEXT:     ACCGangClause
      // DMP-NEXT:     ACCReductionClause {{.*}} '+'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'out0' 'int'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'out1' 'int'
      // DMP-NEXT:     ACCPrivateClause
      // DMP-NEXT:       DeclRefExpr {{.*}} 'out2' 'int'
      // DMP-NEXT:     ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:     impl: OMPDistributeDirective
      // DMP-NEXT:       OMPPrivateClause
      // DMP-NOT:          <implicit>
      // DMP-NEXT:         DeclRefExpr {{.*}} 'out2' 'int'
      // DMP-NOT:        OMP
      // DMP:            ForStmt
      // DMP:          ACCLoopDirective
      // DMP-NEXT:       ACCWorkerClause
      // DMP-NEXT:       ACCReductionClause {{.*}} '+'
      // DMP-NEXT:         DeclRefExpr {{.*}} 'out0' 'int'
      // DMP-NEXT:         DeclRefExpr {{.*}} 'out1' 'int'
      // DMP-NEXT:         DeclRefExpr {{.*}} 'out2' 'int'
      // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:       impl: OMPParallelForDirective
      // DMP-NEXT:         OMPReductionClause
      // DMP-NOT:            <implicit>
      // DMP-NEXT:           DeclRefExpr {{.*}} 'out0' 'int'
      // DMP-NEXT:           DeclRefExpr {{.*}} 'out1' 'int'
      // DMP-NEXT:           DeclRefExpr {{.*}} 'out2' 'int'
      // DMP-NOT:          OMP
      // DMP:              ForStmt
      // DMP:                CompoundAssignOperator {{.*}} '+='
      //
      // PRT-NOACC-NEXT: {{^ *}}for (int i = 0; i < 2; ++i) {
      // PRT-NOACC-NEXT: {{^ *}}  for (int j = 0; j < 2; ++j) {
      // PRT-NOACC-NEXT: {{^ *}}    for (int k = 0; k < 2; ++k) {
      // PRT-NOACC-NEXT: {{^ *}}      out0 += 2;
      // PRT-NOACC-NEXT: {{^ *}}      out1 += 2;
      // PRT-NOACC-NEXT: {{^ *}}      out2 += 2;
      // PRT-NOACC-NEXT: {{^ *}}    }
      // PRT-NOACC-NEXT: {{^ *}}  }
      // PRT-NOACC-NEXT: {{^ *}}}
      //
      // PRT-AO-NEXT: {{^ *}}// v----------ACC----------v
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop seq private(out0) reduction(+: out1){{$}}
      // PRT-A-NEXT:  {{^ *}}for (int i = 0; i < 2; ++i) {
      // PRT-A-NEXT:  {{^ *}}  #pragma acc loop gang reduction(+: out0,out1) private(out2){{$}}
      // PRT-A-NEXT:  {{^ *}}  for (int j = 0; j < 2; ++j) {
      // PRT-A-NEXT:  {{^ *}}    #pragma acc loop worker reduction(+: out0,out1,out2){{$}}
      // PRT-A-NEXT:  {{^ *}}    for (int k = 0; k < 2; ++k) {
      // PRT-A-NEXT:  {{^ *}}      out0 += 2;
      // PRT-A-NEXT:  {{^ *}}      out1 += 2;
      // PRT-A-NEXT:  {{^ *}}      out2 += 2;
      // PRT-A-NEXT:  {{^ *}}    }
      // PRT-A-NEXT:  {{^ *}}  }
      // PRT-A-NEXT:  {{^ *}}}
      // PRT-AO-NEXT: {{^ *}}// ---------ACC->OMP--------
      // PRT-AO-NEXT: {{^ *}}// {
      // PRT-AO-NEXT: {{^ *}}//   int out0;
      // PRT-AO-NEXT: {{^ *}}//   for (int i = 0; i < 2; ++i) {
      // PRT-AO-NEXT: {{^ *}}//     #pragma omp distribute private(out2){{$}}
      // PRT-AO-NEXT: {{^ *}}//     for (int j = 0; j < 2; ++j) {
      // PRT-AO-NEXT: {{^ *}}//       #pragma omp parallel for reduction(+: out0,out1,out2){{$}}
      // PRT-AO-NEXT: {{^ *}}//       for (int k = 0; k < 2; ++k) {
      // PRT-AO-NEXT: {{^ *}}//         out0 += 2;
      // PRT-AO-NEXT: {{^ *}}//         out1 += 2;
      // PRT-AO-NEXT: {{^ *}}//         out2 += 2;
      // PRT-AO-NEXT: {{^ *}}//       }
      // PRT-AO-NEXT: {{^ *}}//     }
      // PRT-AO-NEXT: {{^ *}}//   }
      // PRT-AO-NEXT: {{^ *}}// }
      // PRT-AO-NEXT: {{^ *}}// ^----------OMP----------^
      //
      // PRT-OA-NEXT: {{^ *}}// v----------OMP----------v
      // PRT-O-NEXT:  {{^ *}}{
      // PRT-O-NEXT:  {{^ *}}  int out0;
      // PRT-O-NEXT:  {{^ *}}  for (int i = 0; i < 2; ++i) {
      // PRT-O-NEXT:  {{^ *}}    #pragma omp distribute private(out2){{$}}
      // PRT-O-NEXT:  {{^ *}}    for (int j = 0; j < 2; ++j) {
      // PRT-O-NEXT:  {{^ *}}      #pragma omp parallel for reduction(+: out0,out1,out2){{$}}
      // PRT-O-NEXT:  {{^ *}}      for (int k = 0; k < 2; ++k) {
      // PRT-O-NEXT:  {{^ *}}        out0 += 2;
      // PRT-O-NEXT:  {{^ *}}        out1 += 2;
      // PRT-O-NEXT:  {{^ *}}        out2 += 2;
      // PRT-O-NEXT:  {{^ *}}      }
      // PRT-O-NEXT:  {{^ *}}    }
      // PRT-O-NEXT:  {{^ *}}  }
      // PRT-O-NEXT:  {{^ *}}}
      // PRT-OA-NEXT: {{^ *}}// ---------OMP<-ACC--------
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop seq private(out0) reduction(+: out1){{$}}
      // PRT-OA-NEXT: {{^ *}}// for (int i = 0; i < 2; ++i) {
      // PRT-OA-NEXT: {{^ *}}//   #pragma acc loop gang reduction(+: out0,out1) private(out2){{$}}
      // PRT-OA-NEXT: {{^ *}}//   for (int j = 0; j < 2; ++j) {
      // PRT-OA-NEXT: {{^ *}}//     #pragma acc loop worker reduction(+: out0,out1,out2){{$}}
      // PRT-OA-NEXT: {{^ *}}//     for (int k = 0; k < 2; ++k) {
      // PRT-OA-NEXT: {{^ *}}//       out0 += 2;
      // PRT-OA-NEXT: {{^ *}}//       out1 += 2;
      // PRT-OA-NEXT: {{^ *}}//       out2 += 2;
      // PRT-OA-NEXT: {{^ *}}//     }
      // PRT-OA-NEXT: {{^ *}}//   }
      // PRT-OA-NEXT: {{^ *}}// }
      // PRT-OA-NEXT: {{^ *}}// ^----------ACC----------^
      #pragma acc loop seq private(out0) reduction(+: out1)
      for (int i = 0; i < 2; ++i) {
        #pragma acc loop gang reduction(+: out0,out1) private(out2)
        for (int j = 0; j < 2; ++j) {
          #pragma acc loop worker reduction(+: out0,out1,out2)
          for (int k = 0; k < 2; ++k) {
            out0 += 2;
            out1 += 2;
            out2 += 2;
          }
        }
      }
    } // PRT-NEXT: }
    // DMP: CallExpr
    // DMP: CallExpr
    // DMP: CallExpr
    // PRT-NEXT: printf
    // PRT-NEXT: printf
    // PRT-NEXT: printf
    // EXE-NEXT: out0 = 5{{$}}
    // EXE-NEXT: out1 = 6{{$}}
    // EXE-NEXT: out2 = 7{{$}}
    printf("out0 = %d\n", out0);
    printf("out1 = %d\n", out1);
    printf("out2 = %d\n", out2);
  } // PRT-NEXT: }

  // This is the same as the previous test except the outer acc loop is now
  // combined with the enclosing acc parallel.  Thus, out1's reduction on the
  // outer acc loop now implies a copy clause for out1 on the acc parallel,
  // making out1 a gang-shared variable at the outer acc loop's out1 reduction,
  // which thus becomes a gang reduction.  out0 and out2 still have no gang
  // reductions for the same reasons as in the previous test.
  //
  // Also, the explicit copy has been replaced with copyin, which still
  // shouldn't have any observable effect.

  // PRT-NEXT: {
  {
    // PRT-NEXT: int out0 = 5;
    // PRT-NEXT: int out1 = 6;
    // PRT-NEXT: int out2 = 7;
    int out0 = 5;
    int out1 = 6;
    int out2 = 7;
    // DMP:      ACCParallelLoopDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCSeqClause
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out0' 'int'
    // DMP-NEXT:   ACCReductionClause {{.*}} '+'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out1' 'int'
    // DMP-NEXT:   ACCCopyinClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out2' 'int'
    // DMP-NEXT:   effect: ACCParallelDirective
    // DMP-NEXT:     ACCNumGangsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     ACCCopyinClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out2' 'int'
    // DMP-NEXT:     ACCCopyClause {{.*}} <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out1' 'int'
    // DMP-NEXT:     ACCReductionClause {{.*}} <implicit> '+'
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out1' 'int'
    // DMP-NEXT:     impl: OMPTargetTeamsDirective
    // DMP-NEXT:       OMPNum_teamsClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:       OMPMapClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out2' 'int'
    // DMP-NEXT:       OMPMapClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out1' 'int'
    // DMP-NEXT:       OMPReductionClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out1' 'int'
    // DMP-NOT:        OMP
    // DMP:            CompoundStmt
    // DMP:          ACCLoopDirective
    // DMP-NEXT:       ACCSeqClause
    // DMP-NEXT:       ACCPrivateClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out0' 'int'
    // DMP-NEXT:       ACCReductionClause {{.*}} '+'
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out1' 'int'
    // DMP-NEXT:       impl: CompoundStmt
    // DMP-NEXT:         DeclStmt
    // DMP-NEXT:           VarDecl {{.*}} out0 'int'
    // DMP-NEXT:         ForStmt
    // DMP:            ACCLoopDirective
    // DMP-NEXT:         ACCGangClause
    // DMP-NEXT:         ACCReductionClause {{.*}} '+'
    // DMP-NEXT:           DeclRefExpr {{.*}} 'out0' 'int'
    // DMP-NEXT:           DeclRefExpr {{.*}} 'out1' 'int'
    // DMP-NEXT:         ACCPrivateClause
    // DMP-NEXT:           DeclRefExpr {{.*}} 'out2' 'int'
    // DMP-NEXT:         ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:         impl: OMPDistributeDirective
    // DMP-NEXT:           OMPPrivateClause
    // DMP-NOT:              <implicit>
    // DMP-NEXT:             DeclRefExpr {{.*}} 'out2' 'int'
    // DMP-NOT:            OMP
    // DMP:                ForStmt
    // DMP:              ACCLoopDirective
    // DMP-NEXT:           ACCWorkerClause
    // DMP-NEXT:           ACCReductionClause {{.*}} '+'
    // DMP-NEXT:             DeclRefExpr {{.*}} 'out0' 'int'
    // DMP-NEXT:             DeclRefExpr {{.*}} 'out1' 'int'
    // DMP-NEXT:             DeclRefExpr {{.*}} 'out2' 'int'
    // DMP-NEXT:           ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:           impl: OMPParallelForDirective
    // DMP-NEXT:             OMPReductionClause
    // DMP-NOT:                <implicit>
    // DMP-NEXT:               DeclRefExpr {{.*}} 'out0' 'int'
    // DMP-NEXT:               DeclRefExpr {{.*}} 'out1' 'int'
    // DMP-NEXT:               DeclRefExpr {{.*}} 'out2' 'int'
    // DMP-NOT:              OMP
    // DMP:                  ForStmt
    // DMP:                    CompoundAssignOperator {{.*}} '+='
    //
    // PRT-NOACC-NEXT: {{^ *}}for (int i = 0; i < 2; ++i) {
    // PRT-NOACC-NEXT: {{^ *}}  for (int j = 0; j < 2; ++j) {
    // PRT-NOACC-NEXT: {{^ *}}    for (int k = 0; k < 2; ++k) {
    // PRT-NOACC-NEXT: {{^ *}}      out0 += 2;
    // PRT-NOACC-NEXT: {{^ *}}      out1 += 2;
    // PRT-NOACC-NEXT: {{^ *}}      out2 += 2;
    // PRT-NOACC-NEXT: {{^ *}}    }
    // PRT-NOACC-NEXT: {{^ *}}  }
    // PRT-NOACC-NEXT: {{^ *}}}
    //
    // PRT-AO-NEXT: {{^ *}}// v----------ACC----------v
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel loop num_gangs(2) seq private(out0) reduction(+: out1) copyin(out2){{$}}
    // PRT-A-NEXT:  {{^ *}}for (int i = 0; i < 2; ++i) {
    // PRT-A-NEXT:  {{^ *}}  #pragma acc loop gang reduction(+: out0,out1) private(out2){{$}}
    // PRT-A-NEXT:  {{^ *}}  for (int j = 0; j < 2; ++j) {
    // PRT-A-NEXT:  {{^ *}}    #pragma acc loop worker reduction(+: out0,out1,out2){{$}}
    // PRT-A-NEXT:  {{^ *}}    for (int k = 0; k < 2; ++k) {
    // PRT-A-NEXT:  {{^ *}}      out0 += 2;
    // PRT-A-NEXT:  {{^ *}}      out1 += 2;
    // PRT-A-NEXT:  {{^ *}}      out2 += 2;
    // PRT-A-NEXT:  {{^ *}}    }
    // PRT-A-NEXT:  {{^ *}}  }
    // PRT-A-NEXT:  {{^ *}}}
    // PRT-AO-NEXT: {{^ *}}// ---------ACC->OMP--------
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) map(ompx_hold,to: out2) map(ompx_hold,tofrom: out1) reduction(+: out1){{$}}
    // PRT-AO-NEXT: {{^ *}}// {
    // PRT-AO-NEXT: {{^ *}}//   int out0;
    // PRT-AO-NEXT: {{^ *}}//   for (int i = 0; i < 2; ++i) {
    // PRT-AO-NEXT: {{^ *}}//     #pragma omp distribute private(out2){{$}}
    // PRT-AO-NEXT: {{^ *}}//     for (int j = 0; j < 2; ++j) {
    // PRT-AO-NEXT: {{^ *}}//       #pragma omp parallel for reduction(+: out0,out1,out2){{$}}
    // PRT-AO-NEXT: {{^ *}}//       for (int k = 0; k < 2; ++k) {
    // PRT-AO-NEXT: {{^ *}}//         out0 += 2;
    // PRT-AO-NEXT: {{^ *}}//         out1 += 2;
    // PRT-AO-NEXT: {{^ *}}//         out2 += 2;
    // PRT-AO-NEXT: {{^ *}}//       }
    // PRT-AO-NEXT: {{^ *}}//     }
    // PRT-AO-NEXT: {{^ *}}//   }
    // PRT-AO-NEXT: {{^ *}}// }
    // PRT-AO-NEXT: {{^ *}}// ^----------OMP----------^
    //
    // PRT-OA-NEXT: {{^ *}}// v----------OMP----------v
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) map(ompx_hold,to: out2) map(ompx_hold,tofrom: out1) reduction(+: out1){{$}}
    // PRT-O-NEXT:  {{^ *}}{
    // PRT-O-NEXT:  {{^ *}}  int out0;
    // PRT-O-NEXT:  {{^ *}}  for (int i = 0; i < 2; ++i) {
    // PRT-O-NEXT:  {{^ *}}    #pragma omp distribute private(out2){{$}}
    // PRT-O-NEXT:  {{^ *}}    for (int j = 0; j < 2; ++j) {
    // PRT-O-NEXT:  {{^ *}}      #pragma omp parallel for reduction(+: out0,out1,out2){{$}}
    // PRT-O-NEXT:  {{^ *}}      for (int k = 0; k < 2; ++k) {
    // PRT-O-NEXT:  {{^ *}}        out0 += 2;
    // PRT-O-NEXT:  {{^ *}}        out1 += 2;
    // PRT-O-NEXT:  {{^ *}}        out2 += 2;
    // PRT-O-NEXT:  {{^ *}}      }
    // PRT-O-NEXT:  {{^ *}}    }
    // PRT-O-NEXT:  {{^ *}}  }
    // PRT-O-NEXT:  {{^ *}}}
    // PRT-OA-NEXT: {{^ *}}// ---------OMP<-ACC--------
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) seq private(out0) reduction(+: out1) copyin(out2){{$}}
    // PRT-OA-NEXT: {{^ *}}// for (int i = 0; i < 2; ++i) {
    // PRT-OA-NEXT: {{^ *}}//   #pragma acc loop gang reduction(+: out0,out1) private(out2){{$}}
    // PRT-OA-NEXT: {{^ *}}//   for (int j = 0; j < 2; ++j) {
    // PRT-OA-NEXT: {{^ *}}//     #pragma acc loop worker reduction(+: out0,out1,out2){{$}}
    // PRT-OA-NEXT: {{^ *}}//     for (int k = 0; k < 2; ++k) {
    // PRT-OA-NEXT: {{^ *}}//       out0 += 2;
    // PRT-OA-NEXT: {{^ *}}//       out1 += 2;
    // PRT-OA-NEXT: {{^ *}}//       out2 += 2;
    // PRT-OA-NEXT: {{^ *}}//     }
    // PRT-OA-NEXT: {{^ *}}//   }
    // PRT-OA-NEXT: {{^ *}}// }
    // PRT-OA-NEXT: {{^ *}}// ^----------ACC----------^
    #pragma acc parallel loop num_gangs(2) seq private(out0) reduction(+: out1) copyin(out2)
    for (int i = 0; i < 2; ++i) {
      #pragma acc loop gang reduction(+: out0,out1) private(out2)
      for (int j = 0; j < 2; ++j) {
        #pragma acc loop worker reduction(+: out0,out1,out2)
        for (int k = 0; k < 2; ++k) {
          out0 += 2;
          out1 += 2;
          out2 += 2;
        }
      }
    }
    // DMP: CallExpr
    // DMP: CallExpr
    // DMP: CallExpr
    // PRT-NEXT: printf
    // PRT-NEXT: printf
    // PRT-NEXT: printf
    // EXE-NEXT: out0 = 5{{$}}
    // EXE-NEXT: out1 = 22{{$}}
    // EXE-NEXT: out2 = 7{{$}}
    printf("out0 = %d\n", out0);
    printf("out1 = %d\n", out1);
    printf("out2 = %d\n", out2);
  } // PRT-NEXT: }

  //----------------------------------------------------------------------------
  // Check within accelerator routines.
  //----------------------------------------------------------------------------

  //   EXE-NOT: {{.}}
  // EXE-LABEL: within accelerator routines
  //   EXE-NOT: {{.}}
  printf("within accelerator routines\n");

  #pragma acc parallel num_gangs(2)
  {
    withinGangFn(0);
    withinWorkerFn(0);
    withinVectorFn(0);
    withinSeqFn(0);
  }

  return 0;
}

//==============================================================================
// Check within gang functions.
//
// This repeats some of the above testing but within a gang function and without
// statically enclosing parallel constructs.
//==============================================================================

// DMP-LABEL: FunctionDecl {{.*}} withinGangFn
// PRT-LABEL: static void withinGangFn(int param) {
static void withinGangFn(int param) {
  // PRT-NEXT: int local;
  int local;

  //----------------------------------------------------------------------------
  // Explicit seq.
  //----------------------------------------------------------------------------

  // PRT-NEXT: param = 10;
  // PRT-NEXT: local = 20;
  param = 10;
  local = 20;
  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCSeqClause
  // DMP-NEXT:   ACCReductionClause {{.*}} '*'
  // DMP-NEXT:     DeclRefExpr {{.*}} 'param' 'int'
  // DMP-NEXT:   ACCReductionClause {{.*}} '+'
  // DMP-NEXT:     DeclRefExpr {{.*}} 'local' 'int'
  // DMP-NEXT:   impl: ForStmt
  //      DMP:     CompoundAssignOperator {{.*}} 'int' '*='
  //      DMP:     CompoundAssignOperator {{.*}} 'int' '+='
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop seq reduction(*: param) reduction(+: local)
  // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
  //  PRT-A-SAME: {{^$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop seq reduction(*: param) reduction(+: local) // discarded in OpenMP translation{{$}}
  //    PRT-NEXT: for ({{.*}}) {
  //    PRT-NEXT:   param *= 2;
  //    PRT-NEXT:   local += 2;
  //    PRT-NEXT: }
  #pragma acc loop seq reduction(*: param) reduction(+: local)
  for (int i = 0; i < 2; ++i) {
    param *= 2;
    local += 2;
  }
  // PRT-NEXT: {{TGT_PRINTF|printf}}
  // PRT-NEXT: {{TGT_PRINTF|printf}}
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: explicit seq: param = 40{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: explicit seq: param = 40{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: explicit seq: local = 24{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: explicit seq: local = 24{{$}}
  TGT_PRINTF("withinGangFn: explicit seq: param = %d\n", param);
  TGT_PRINTF("withinGangFn: explicit seq: local = %d\n", local);

  //----------------------------------------------------------------------------
  // Implicit gang.
  //----------------------------------------------------------------------------

  // PRT-NEXT: {
  {
    // PRT-NEXT: int arr[] = {10000, 1};
    // PRT-NEXT: int *min = arr;
    // PRT-NEXT: int *max = arr;
    int arr[] = {10000, 1};
    int *min = arr;
    int *max = arr;
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCReductionClause {{.*}} 'min'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'min' 'int *'
    // DMP-NEXT:   ACCReductionClause {{.*}} 'max'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'max' 'int *'
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'arr' 'int[2]'
    // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPDistributeDirective
    //  DMP-NOT:     OMP
    //      DMP:     ForStmt
    //      DMP:       ConditionalOperator
    //      DMP:       ConditionalOperator
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop reduction(min: min) reduction(max: max){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute{{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop reduction(min: min) reduction(max: max){{$}}
    //    PRT-NEXT: for ({{.*}}) {
    //    PRT-NEXT:   min = arr + i < min ? arr + i : min;
    //    PRT-NEXT:   max = arr + i > max ? arr + i : max;
    //    PRT-NEXT: }
    #pragma acc loop reduction(min: min) reduction(max: max)
    for (int i = 0; i < 2; ++i) {
      min = arr + i < min ? arr + i : min;
      max = arr + i > max ? arr + i : max;
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: implicit gang: min == arr: 1
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: implicit gang: min == arr: 1
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: implicit gang: max == arr: 1
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: implicit gang: max == arr: 0
    TGT_PRINTF("withinGangFn: implicit gang: min == arr: %d\n", min == arr);
    TGT_PRINTF("withinGangFn: implicit gang: max == arr: %d\n", max == arr);
  } // PRT-NEXT: }

  //----------------------------------------------------------------------------
  // auto with partitioning.
  //----------------------------------------------------------------------------

  // PRT-NEXT: {
  {
    // PRT-NEXT: _Bool b = 1;
    // PRT-NEXT: param = 10;
    _Bool b = 1;
    param = 10;
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCAutoClause
    // DMP-NEXT:   ACCWorkerClause
    // DMP-NEXT:   ACCReductionClause {{.*}} '&'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'b' '_Bool'
    // DMP-NEXT:   ACCReductionClause {{.*}} '|'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'param' 'int'
    // DMP-NEXT:   impl: ForStmt
    //      DMP:     CompoundAssignOperator {{.*}} '_Bool' '&='
    //      DMP:     CompoundAssignOperator {{.*}} 'int' '|='
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop auto worker reduction(&: b) reduction(|: param)
    // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
    //  PRT-A-SAME: {{^$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop auto worker reduction(&: b) reduction(|: param) // discarded in OpenMP translation{{$}}
    //    PRT-NEXT: for ({{.*}}) {
    //    PRT-NEXT:   b &= i;
    //    PRT-NEXT:   param |= i;
    //    PRT-NEXT: }
    #pragma acc loop auto worker reduction(&: b) reduction(|: param)
    for (int i = 0; i < 2; ++i) {
      b &= i;
      param |= i;
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: auto with partitioning: b = 0{{$}}
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: auto with partitioning: b = 0{{$}}
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: auto with partitioning: param = 11{{$}}
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: auto with partitioning: param = 11{{$}}
    TGT_PRINTF("withinGangFn: auto with partitioning: b = %d\n", b);
    TGT_PRINTF("withinGangFn: auto with partitioning: param = %d\n", param);
  } // PRT-NEXT: }

  //----------------------------------------------------------------------------
  // worker and implicit gang.
  //
  // FIXME: OpenMP offloading nvptx64 doesn't seem to support _Complex
  // properly.  When the host is x86_64, we see incorrect values printed within
  // the following kernel.  When the host is ppc64le, the following produces a
  // runtime error:
  //
  //   Libomptarget fatal error 1: failure of target construct while
  //   offloading is mandatory
  //
  // FIXME: We couldn't include complex.h for amdgcn (see above).
  //----------------------------------------------------------------------------

// PRT-SRC-NEXT: #if !TGT_NVPTX64 && !TGT_AMDGCN
#if !TGT_NVPTX64 && !TGT_AMDGCN
  // PRT-NEXT: {
  {
    // PRT-NEXT: _Complex double c = 2;
    // PRT-NEXT: param = 0;
    _Complex double c = 2;
    param = 0;
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCWorkerClause
    // DMP-NEXT:   ACCReductionClause {{.*}} '&&'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'c' '_Complex double'
    // DMP-NEXT:   ACCReductionClause {{.*}} '||'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'param' 'int'
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPDistributeParallelForDirective
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'c' '_Complex double'
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'param' 'int'
    //      DMP:     BinaryOperator {{.*}} '&&'
    //      DMP:     BinaryOperator {{.*}} '||'
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop worker reduction(&&: c) reduction(||: param){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for reduction(&&: c) reduction(||: param){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute parallel for reduction(&&: c) reduction(||: param){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker reduction(&&: c) reduction(||: param){{$}}
    //    PRT-NEXT: for ({{.*}}) {
    //    PRT-NEXT:   c = c && 0;
    //    PRT-NEXT:   param = param || i;
    //    PRT-NEXT: }
    #pragma acc loop worker reduction(&&: c) reduction(||: param)
    for (int i = 0; i < 2; ++i) {
      c = c && 0;
      param = param || i;
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    //        EXE-HOST-DAG: withinGangFn: worker and implicit gang: c: 0.0 + 0.0i
    //        EXE-HOST-DAG: withinGangFn: worker and implicit gang: c: 0.0 + 0.0i
    //  EXE-TGT-X86_64-DAG: withinGangFn: worker and implicit gang: c: 0.0 + 0.0i
    //  EXE-TGT-X86_64-DAG: withinGangFn: worker and implicit gang: c: 0.0 + 0.0i
    // EXE-TGT-PPC64LE-DAG: withinGangFn: worker and implicit gang: c: 0.0 + 0.0i
    // EXE-TGT-PPC64LE-DAG: withinGangFn: worker and implicit gang: c: 0.0 + 0.0i
    //        EXE-HOST-DAG: withinGangFn: worker and implicit gang: param: 0
    //        EXE-HOST-DAG: withinGangFn: worker and implicit gang: param: 1
    //  EXE-TGT-X86_64-DAG: withinGangFn: worker and implicit gang: param: 0
    //  EXE-TGT-X86_64-DAG: withinGangFn: worker and implicit gang: param: 1
    // EXE-TGT-PPC64LE-DAG: withinGangFn: worker and implicit gang: param: 0
    // EXE-TGT-PPC64LE-DAG: withinGangFn: worker and implicit gang: param: 1
    TGT_PRINTF("withinGangFn: worker and implicit gang: c: %.1f + %.1fi\n", creal(c), cimag(c));
    TGT_PRINTF("withinGangFn: worker and implicit gang: param: %d\n", param);
  } // PRT-NEXT: }
// PRT-SRC-NEXT: #endif
#endif

  //----------------------------------------------------------------------------
  // vector and implicit gang.
  //----------------------------------------------------------------------------

  // PRT-NEXT: {
  {
    // PRT-NEXT: long l = 5;
    long l = 5;
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCVectorClause
    // DMP-NEXT:   ACCReductionClause {{.*}} '+
    // DMP-NEXT:     DeclRefExpr {{.*}} 'l' 'long'
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPDistributeSimdDirective
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'l' 'long'
    //      DMP:     CompoundAssignOperator {{.*}} '+='
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop vector reduction(+: l){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute simd reduction(+: l){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute simd reduction(+: l){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop vector reduction(+: l){{$}}
    //    PRT-NEXT: for ({{.*}})
    //    PRT-NEXT:   l += 10;
    #pragma acc loop vector reduction(+: l)
    for (int i = 0; i < 2; ++i)
      l += 10;
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: vector and implicit gang: l = {{15|5}}{{$}}
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: vector and implicit gang: l = {{15|25}}{{$}}
    TGT_PRINTF("withinGangFn: vector and implicit gang: l = %ld\n", l);
  } // PRT-NEXT: }

  //----------------------------------------------------------------------------
  // worker, vector, and implicit gang.
  //----------------------------------------------------------------------------

  // PRT-NEXT: {
  {
    // PRT-NEXT: float f = -7;
    // PRT-NEXT: param = 3;
    float f = -7;
    param = 3;
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCWorkerClause
    // DMP-NEXT:   ACCVectorClause
    // DMP-NEXT:   ACCReductionClause {{.*}} '*'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'f' 'float'
    // DMP-NEXT:   ACCReductionClause {{.*}} '+'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'param' 'int'
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPDistributeParallelForSimdDirective
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'f' 'float'
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'param' 'int'
    //      DMP:     CompoundAssignOperator {{.*}} '*='
    //      DMP:     CompoundAssignOperator {{.*}} '+='
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop worker vector reduction(*: f) reduction(+: param){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for simd reduction(*: f) reduction(+: param){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute parallel for simd reduction(*: f) reduction(+: param){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker vector reduction(*: f) reduction(+: param){{$}}
    //    PRT-NEXT: for ({{.*}}) {
    //    PRT-NEXT:   f *= 3;
    //    PRT-NEXT:   param += 4;
    //    PRT-NEXT: }
    #pragma acc loop worker vector reduction(*: f) reduction(+: param)
    for (int i = 0; i < 2; ++i) {
      f *= 3;
      param += 4;
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: worker, vector, and implicit gang: f = {{-21.0|-7.0}}{{$}}
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: worker, vector, and implicit gang: f = {{-21.0|-63.0}}{{$}}
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: worker, vector, and implicit gang: param = {{7|3}}{{$}}
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: worker, vector, and implicit gang: param = {{7|11}}{{$}}
    TGT_PRINTF("withinGangFn: worker, vector, and implicit gang: f = %.1f\n", f);
    TGT_PRINTF("withinGangFn: worker, vector, and implicit gang: param = %d\n", param);
  } // PRT-NEXT: }

  //----------------------------------------------------------------------------
  // gang, worker, vector, and separate nested loops.
  //----------------------------------------------------------------------------

  // PRT-NEXT: {
  {
    // PRT-NEXT: local = 3;
    local = 3;
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCGangClause
    // DMP-NEXT:   ACCReductionClause {{.*}} '+'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'local' 'int'
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPDistributeDirective
    //  DMP-NOT:     OMPReductionClause
    //      DMP:     Stmt
    //      DMP:     ACCLoopDirective
    // DMP-NEXT:       ACCWorkerClause
    // DMP-NEXT:       ACCReductionClause {{.*}} '+'
    // DMP-NEXT:         DeclRefExpr {{.*}} 'local' 'int'
    // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
    //      DMP:       ACCLoopDirective
    // DMP-NEXT:         ACCVectorClause
    // DMP-NEXT:         ACCReductionClause {{.*}} '+'
    // DMP-NEXT:           DeclRefExpr {{.*}} 'local' 'int'
    // DMP-NEXT:         ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:         impl: OMPSimdDirective
    // DMP-NEXT:           OMPReductionClause
    // DMP-NEXT:             DeclRefExpr {{.*}} 'local' 'int'
    //      DMP:           CompoundAssignOperator {{.*}} '+='
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop gang reduction(+: local){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute{{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang reduction(+: local){{$}}
    //    PRT-NEXT: for ({{.*}}) {
    //  PRT-A-NEXT:   {{^ *}}#pragma acc loop worker reduction(+: local){{$}}
    // PRT-AO-NEXT:   {{^ *}}// #pragma omp parallel for reduction(+: local){{$}}
    //  PRT-O-NEXT:   {{^ *}}#pragma omp parallel for reduction(+: local){{$}}
    // PRT-OA-NEXT:   {{^ *}}// #pragma acc loop worker reduction(+: local){{$}}
    //    PRT-NEXT:   for ({{.*}}) {
    //  PRT-A-NEXT:     {{^ *}}#pragma acc loop vector reduction(+: local){{$}}
    // PRT-AO-NEXT:     {{^ *}}// #pragma omp simd reduction(+: local){{$}}
    //  PRT-O-NEXT:     {{^ *}}#pragma omp simd reduction(+: local){{$}}
    // PRT-OA-NEXT:     {{^ *}}// #pragma acc loop vector reduction(+: local){{$}}
    //    PRT-NEXT:     for ({{.*}})
    //    PRT-NEXT:       local +=
    //    PRT-NEXT:   }
    //    PRT-NEXT: }
    #pragma acc loop gang reduction(+: local)
    for (int i = 0; i < 2; ++i) {
      #pragma acc loop worker reduction(+: local)
      for (int j = 0; j < 2; ++j) {
        #pragma acc loop vector reduction(+: local)
        for (int k = 0; k < 2; ++k)
          local += 2;
      }
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: gang, worker, vector, and separate nested loops: local = {{11|3}}{{$}}
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: gang, worker, vector, and separate nested loops: local = {{11|19}}{{$}}
    TGT_PRINTF("withinGangFn: gang, worker, vector, and separate nested loops: local = %d\n", local);
  } // PRT-NEXT: }
} // PRT-NEXT: }

//==============================================================================
// Check within worker functions.
//
// This repeats some of the above testing but within a worker function.
//==============================================================================

// DMP-LABEL: FunctionDecl {{.*}} withinWorkerFn
// PRT-LABEL: static void withinWorkerFn(int param) {
static void withinWorkerFn(int param) {
  // PRT-NEXT: int local;
  int local;

  //----------------------------------------------------------------------------
  // Explicit seq.
  //----------------------------------------------------------------------------

  // PRT-NEXT: param = 10;
  // PRT-NEXT: local = 20;
  param = 10;
  local = 20;
  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCSeqClause
  // DMP-NEXT:   ACCReductionClause {{.*}} '*'
  // DMP-NEXT:     DeclRefExpr {{.*}} 'param' 'int'
  // DMP-NEXT:   ACCReductionClause {{.*}} '+'
  // DMP-NEXT:     DeclRefExpr {{.*}} 'local' 'int'
  // DMP-NEXT:   impl: ForStmt
  //      DMP:     CompoundAssignOperator {{.*}} 'int' '*='
  //      DMP:     CompoundAssignOperator {{.*}} 'int' '+='
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop seq reduction(*: param) reduction(+: local)
  // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
  //  PRT-A-SAME: {{^$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop seq reduction(*: param) reduction(+: local) // discarded in OpenMP translation{{$}}
  //    PRT-NEXT: for ({{.*}}) {
  //    PRT-NEXT:   param *= 2;
  //    PRT-NEXT:   local += 2;
  //    PRT-NEXT: }
  #pragma acc loop seq reduction(*: param) reduction(+: local)
  for (int i = 0; i < 2; ++i) {
    param *= 2;
    local += 2;
  }
  // PRT-NEXT: {{TGT_PRINTF|printf}}
  // PRT-NEXT: {{TGT_PRINTF|printf}}
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: explicit seq: param = 40{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: explicit seq: param = 40{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: explicit seq: local = 24{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: explicit seq: local = 24{{$}}
  TGT_PRINTF("withinWorkerFn: explicit seq: param = %d\n", param);
  TGT_PRINTF("withinWorkerFn: explicit seq: local = %d\n", local);

  //----------------------------------------------------------------------------
  // Naked loop.
  //----------------------------------------------------------------------------

  // PRT-NEXT: {
  {
    // PRT-NEXT: int arr[] = {10000, 1};
    // PRT-NEXT: int *min = arr;
    // PRT-NEXT: int *max = arr;
    int arr[] = {10000, 1};
    int *min = arr;
    int *max = arr;
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCReductionClause {{.*}} 'min'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'min' 'int *'
    // DMP-NEXT:   ACCReductionClause {{.*}} 'max'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'max' 'int *'
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'arr' 'int[2]'
    // DMP-NEXT:   impl: ForStmt
    //      DMP:       ConditionalOperator
    //      DMP:       ConditionalOperator
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop reduction(min: min) reduction(max: max)
    // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
    //  PRT-A-SAME: {{^$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop reduction(min: min) reduction(max: max) // discarded in OpenMP translation{{$}}
    //    PRT-NEXT: for ({{.*}}) {
    //    PRT-NEXT:   min = arr + i < min ? arr + i : min;
    //    PRT-NEXT:   max = arr + i > max ? arr + i : max;
    //    PRT-NEXT: }
    #pragma acc loop reduction(min: min) reduction(max: max)
    for (int i = 0; i < 2; ++i) {
      min = arr + i < min ? arr + i : min;
      max = arr + i > max ? arr + i : max;
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: naked loop: min == arr: 1
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: naked loop: min == arr: 1
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: naked loop: max == arr: 0
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: naked loop: max == arr: 0
    TGT_PRINTF("withinWorkerFn: naked loop: min == arr: %d\n", min == arr);
    TGT_PRINTF("withinWorkerFn: naked loop: max == arr: %d\n", max == arr);
  } // PRT-NEXT: }

  //----------------------------------------------------------------------------
  // auto with partitioning.
  //----------------------------------------------------------------------------

  // PRT-NEXT: {
  {
    // PRT-NEXT: _Bool b = 1;
    // PRT-NEXT: param = 10;
    _Bool b = 1;
    param = 10;
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCAutoClause
    // DMP-NEXT:   ACCWorkerClause
    // DMP-NEXT:   ACCReductionClause {{.*}} '&'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'b' '_Bool'
    // DMP-NEXT:   ACCReductionClause {{.*}} '|'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'param' 'int'
    // DMP-NEXT:   impl: ForStmt
    //      DMP:     CompoundAssignOperator {{.*}} '_Bool' '&='
    //      DMP:     CompoundAssignOperator {{.*}} 'int' '|='
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop auto worker reduction(&: b) reduction(|: param)
    // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
    //  PRT-A-SAME: {{^$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop auto worker reduction(&: b) reduction(|: param) // discarded in OpenMP translation{{$}}
    //    PRT-NEXT: for ({{.*}}) {
    //    PRT-NEXT:   b &= i;
    //    PRT-NEXT:   param |= i;
    //    PRT-NEXT: }
    #pragma acc loop auto worker reduction(&: b) reduction(|: param)
    for (int i = 0; i < 2; ++i) {
      b &= i;
      param |= i;
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: auto with partitioning: b = 0{{$}}
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: auto with partitioning: b = 0{{$}}
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: auto with partitioning: param = 11{{$}}
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: auto with partitioning: param = 11{{$}}
    TGT_PRINTF("withinWorkerFn: auto with partitioning: b = %d\n", b);
    TGT_PRINTF("withinWorkerFn: auto with partitioning: param = %d\n", param);
  } // PRT-NEXT: }

  //----------------------------------------------------------------------------
  // worker.
  //
  // FIXME: OpenMP offloading nvptx64 doesn't seem to support _Complex
  // properly.  When the host is x86_64, we see incorrect values printed within
  // the following kernel.  When the host is ppc64le, the following produces a
  // runtime error:
  //
  //   Libomptarget fatal error 1: failure of target construct while
  //   offloading is mandatory
  //
  // FIXME: We couldn't include complex.h for amdgcn (see above).
  //----------------------------------------------------------------------------

// PRT-SRC-NEXT: #if !TGT_NVPTX64 && !TGT_AMDGCN
#if !TGT_NVPTX64 && !TGT_AMDGCN
  // PRT-NEXT: {
  {
    // PRT-NEXT: _Complex double c = 2;
    // PRT-NEXT: param = 0;
    _Complex double c = 2;
    param = 0;
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCWorkerClause
    // DMP-NEXT:   ACCReductionClause {{.*}} '&&'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'c' '_Complex double'
    // DMP-NEXT:   ACCReductionClause {{.*}} '||'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'param' 'int'
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPParallelForDirective
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'c' '_Complex double'
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'param' 'int'
    //      DMP:     BinaryOperator {{.*}} '&&'
    //      DMP:     BinaryOperator {{.*}} '||'
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop worker reduction(&&: c) reduction(||: param){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp parallel for reduction(&&: c) reduction(||: param){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp parallel for reduction(&&: c) reduction(||: param){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker reduction(&&: c) reduction(||: param){{$}}
    //    PRT-NEXT: for ({{.*}}) {
    //    PRT-NEXT:   c = c && 0;
    //    PRT-NEXT:   param = param || i;
    //    PRT-NEXT: }
    #pragma acc loop worker reduction(&&: c) reduction(||: param)
    for (int i = 0; i < 2; ++i) {
      c = c && 0;
      param = param || i;
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    //        EXE-HOST-DAG: withinWorkerFn: worker: c: 0.0 + 0.0i
    //        EXE-HOST-DAG: withinWorkerFn: worker: c: 0.0 + 0.0i
    //  EXE-TGT-X86_64-DAG: withinWorkerFn: worker: c: 0.0 + 0.0i
    //  EXE-TGT-X86_64-DAG: withinWorkerFn: worker: c: 0.0 + 0.0i
    // EXE-TGT-PPC64LE-DAG: withinWorkerFn: worker: c: 0.0 + 0.0i
    // EXE-TGT-PPC64LE-DAG: withinWorkerFn: worker: c: 0.0 + 0.0i
    //        EXE-HOST-DAG: withinWorkerFn: worker: param: 1
    //        EXE-HOST-DAG: withinWorkerFn: worker: param: 1
    //  EXE-TGT-X86_64-DAG: withinWorkerFn: worker: param: 1
    //  EXE-TGT-X86_64-DAG: withinWorkerFn: worker: param: 1
    // EXE-TGT-PPC64LE-DAG: withinWorkerFn: worker: param: 1
    // EXE-TGT-PPC64LE-DAG: withinWorkerFn: worker: param: 1
    TGT_PRINTF("withinWorkerFn: worker: c: %.1f + %.1fi\n", creal(c), cimag(c));
    TGT_PRINTF("withinWorkerFn: worker: param: %d\n", param);
  } // PRT-NEXT: }
// PRT-SRC-NEXT: #endif
#endif

  //----------------------------------------------------------------------------
  // vector.
  //----------------------------------------------------------------------------

  // PRT-NEXT: {
  {
    // PRT-NEXT: long l = 5;
    long l = 5;
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCVectorClause
    // DMP-NEXT:   ACCReductionClause {{.*}} '+
    // DMP-NEXT:     DeclRefExpr {{.*}} 'l' 'long'
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPSimdDirective
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'l' 'long'
    //      DMP:     CompoundAssignOperator {{.*}} '+='
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop vector reduction(+: l){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp simd reduction(+: l){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp simd reduction(+: l){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop vector reduction(+: l){{$}}
    //    PRT-NEXT: for ({{.*}})
    //    PRT-NEXT:   l += 10;
    #pragma acc loop vector reduction(+: l)
    for (int i = 0; i < 2; ++i)
      l += 10;
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: vector: l = 25{{$}}
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: vector: l = 25{{$}}
    TGT_PRINTF("withinWorkerFn: vector: l = %ld\n", l);
  } // PRT-NEXT: }

  //----------------------------------------------------------------------------
  // worker and vector.
  //----------------------------------------------------------------------------

  // PRT-NEXT: {
  {
    // PRT-NEXT: float f = -7;
    // PRT-NEXT: param = 3;
    float f = -7;
    param = 3;
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCWorkerClause
    // DMP-NEXT:   ACCVectorClause
    // DMP-NEXT:   ACCReductionClause {{.*}} '*'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'f' 'float'
    // DMP-NEXT:   ACCReductionClause {{.*}} '+'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'param' 'int'
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPParallelForSimdDirective
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'f' 'float'
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'param' 'int'
    //      DMP:     CompoundAssignOperator {{.*}} '*='
    //      DMP:     CompoundAssignOperator {{.*}} '+='
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop worker vector reduction(*: f) reduction(+: param){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp parallel for simd reduction(*: f) reduction(+: param){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp parallel for simd reduction(*: f) reduction(+: param){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker vector reduction(*: f) reduction(+: param){{$}}
    //    PRT-NEXT: for ({{.*}}) {
    //    PRT-NEXT:   f *= 3;
    //    PRT-NEXT:   param += 4;
    //    PRT-NEXT: }
    #pragma acc loop worker vector reduction(*: f) reduction(+: param)
    for (int i = 0; i < 2; ++i) {
      f *= 3;
      param += 4;
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: worker and vector: f = -63.0{{$}}
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: worker and vector: f = -63.0{{$}}
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: worker and vector: param = 11{{$}}
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: worker and vector: param = 11{{$}}
    TGT_PRINTF("withinWorkerFn: worker and vector: f = %.1f\n", f);
    TGT_PRINTF("withinWorkerFn: worker and vector: param = %d\n", param);
  } // PRT-NEXT: }

  //----------------------------------------------------------------------------
  // worker, vector, and separate nested loops.
  //----------------------------------------------------------------------------

  // PRT-NEXT: {
  {
    // PRT-NEXT: local = 3;
    local = 3;
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCWorkerClause
    // DMP-NEXT:   ACCReductionClause {{.*}} '+'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'local' 'int'
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    //      DMP:   ACCLoopDirective
    // DMP-NEXT:     ACCVectorClause
    // DMP-NEXT:     ACCReductionClause {{.*}} '+'
    // DMP-NEXT:       DeclRefExpr {{.*}} 'local' 'int'
    // DMP-NEXT:     ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:     impl: OMPSimdDirective
    // DMP-NEXT:       OMPReductionClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'local' 'int'
    //      DMP:       CompoundAssignOperator {{.*}} '+='
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop worker reduction(+: local){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp parallel for reduction(+: local){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp parallel for reduction(+: local){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker reduction(+: local){{$}}
    //    PRT-NEXT: for ({{.*}}) {
    //  PRT-A-NEXT:   {{^ *}}#pragma acc loop vector reduction(+: local){{$}}
    // PRT-AO-NEXT:   {{^ *}}// #pragma omp simd reduction(+: local){{$}}
    //  PRT-O-NEXT:   {{^ *}}#pragma omp simd reduction(+: local){{$}}
    // PRT-OA-NEXT:   {{^ *}}// #pragma acc loop vector reduction(+: local){{$}}
    //    PRT-NEXT:   for ({{.*}})
    //    PRT-NEXT:     local +=
    //    PRT-NEXT: }
    #pragma acc loop worker reduction(+: local)
    for (int j = 0; j < 2; ++j) {
      #pragma acc loop vector reduction(+: local)
      for (int k = 0; k < 2; ++k)
        local += 2;
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: worker, vector, and separate nested loops: local = 11{{$}}
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: worker, vector, and separate nested loops: local = 11{{$}}
    TGT_PRINTF("withinWorkerFn: worker, vector, and separate nested loops: local = %d\n", local);
  } // PRT-NEXT: }
} // PRT-NEXT: }

//==============================================================================
// Check within vector functions.
//
// This repeats some of the above testing but within a vector function.
//==============================================================================

// DMP-LABEL: FunctionDecl {{.*}} withinVectorFn
// PRT-LABEL: static void withinVectorFn(int param) {
static void withinVectorFn(int param) {
  // PRT-NEXT: int local;
  int local;

  //----------------------------------------------------------------------------
  // Explicit seq.
  //----------------------------------------------------------------------------

  // PRT-NEXT: param = 10;
  // PRT-NEXT: local = 20;
  param = 10;
  local = 20;
  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCSeqClause
  // DMP-NEXT:   ACCReductionClause {{.*}} '*'
  // DMP-NEXT:     DeclRefExpr {{.*}} 'param' 'int'
  // DMP-NEXT:   ACCReductionClause {{.*}} '+'
  // DMP-NEXT:     DeclRefExpr {{.*}} 'local' 'int'
  // DMP-NEXT:   impl: ForStmt
  //      DMP:     CompoundAssignOperator {{.*}} 'int' '*='
  //      DMP:     CompoundAssignOperator {{.*}} 'int' '+='
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop seq reduction(*: param) reduction(+: local)
  // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
  //  PRT-A-SAME: {{^$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop seq reduction(*: param) reduction(+: local) // discarded in OpenMP translation{{$}}
  //    PRT-NEXT: for ({{.*}}) {
  //    PRT-NEXT:   param *= 2;
  //    PRT-NEXT:   local += 2;
  //    PRT-NEXT: }
  #pragma acc loop seq reduction(*: param) reduction(+: local)
  for (int i = 0; i < 2; ++i) {
    param *= 2;
    local += 2;
  }
  // PRT-NEXT: {{TGT_PRINTF|printf}}
  // PRT-NEXT: {{TGT_PRINTF|printf}}
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: explicit seq: param = 40{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: explicit seq: param = 40{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: explicit seq: local = 24{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: explicit seq: local = 24{{$}}
  TGT_PRINTF("withinVectorFn: explicit seq: param = %d\n", param);
  TGT_PRINTF("withinVectorFn: explicit seq: local = %d\n", local);

  //----------------------------------------------------------------------------
  // Naked loop.
  //----------------------------------------------------------------------------

  // PRT-NEXT: {
  {
    // PRT-NEXT: int arr[] = {10000, 1};
    // PRT-NEXT: int *min = arr;
    // PRT-NEXT: int *max = arr;
    int arr[] = {10000, 1};
    int *min = arr;
    int *max = arr;
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCReductionClause {{.*}} 'min'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'min' 'int *'
    // DMP-NEXT:   ACCReductionClause {{.*}} 'max'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'max' 'int *'
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'arr' 'int[2]'
    // DMP-NEXT:   impl: ForStmt
    //      DMP:       ConditionalOperator
    //      DMP:       ConditionalOperator
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop reduction(min: min) reduction(max: max)
    // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
    //  PRT-A-SAME: {{^$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop reduction(min: min) reduction(max: max) // discarded in OpenMP translation{{$}}
    //    PRT-NEXT: for ({{.*}}) {
    //    PRT-NEXT:   min = arr + i < min ? arr + i : min;
    //    PRT-NEXT:   max = arr + i > max ? arr + i : max;
    //    PRT-NEXT: }
    #pragma acc loop reduction(min: min) reduction(max: max)
    for (int i = 0; i < 2; ++i) {
      min = arr + i < min ? arr + i : min;
      max = arr + i > max ? arr + i : max;
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: withinVectorFn: naked loop: min == arr: 1
    // EXE-TGT-USE-STDIO-DAG: withinVectorFn: naked loop: min == arr: 1
    // EXE-TGT-USE-STDIO-DAG: withinVectorFn: naked loop: max == arr: 0
    // EXE-TGT-USE-STDIO-DAG: withinVectorFn: naked loop: max == arr: 0
    TGT_PRINTF("withinVectorFn: naked loop: min == arr: %d\n", min == arr);
    TGT_PRINTF("withinVectorFn: naked loop: max == arr: %d\n", max == arr);
  } // PRT-NEXT: }

  //----------------------------------------------------------------------------
  // auto with partitioning.
  //----------------------------------------------------------------------------

  // PRT-NEXT: {
  {
    // PRT-NEXT: _Bool b = 1;
    // PRT-NEXT: param = 10;
    _Bool b = 1;
    param = 10;
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCAutoClause
    // DMP-NEXT:   ACCVectorClause
    // DMP-NEXT:   ACCReductionClause {{.*}} '&'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'b' '_Bool'
    // DMP-NEXT:   ACCReductionClause {{.*}} '|'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'param' 'int'
    // DMP-NEXT:   impl: ForStmt
    //      DMP:     CompoundAssignOperator {{.*}} '_Bool' '&='
    //      DMP:     CompoundAssignOperator {{.*}} 'int' '|='
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop auto vector reduction(&: b) reduction(|: param)
    // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
    //  PRT-A-SAME: {{^$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop auto vector reduction(&: b) reduction(|: param) // discarded in OpenMP translation{{$}}
    //    PRT-NEXT: for ({{.*}}) {
    //    PRT-NEXT:   b &= i;
    //    PRT-NEXT:   param |= i;
    //    PRT-NEXT: }
    #pragma acc loop auto vector reduction(&: b) reduction(|: param)
    for (int i = 0; i < 2; ++i) {
      b &= i;
      param |= i;
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: withinVectorFn: auto with partitioning: b = 0{{$}}
    // EXE-TGT-USE-STDIO-DAG: withinVectorFn: auto with partitioning: b = 0{{$}}
    // EXE-TGT-USE-STDIO-DAG: withinVectorFn: auto with partitioning: param = 11{{$}}
    // EXE-TGT-USE-STDIO-DAG: withinVectorFn: auto with partitioning: param = 11{{$}}
    TGT_PRINTF("withinVectorFn: auto with partitioning: b = %d\n", b);
    TGT_PRINTF("withinVectorFn: auto with partitioning: param = %d\n", param);
  } // PRT-NEXT: }

  //----------------------------------------------------------------------------
  // vector.
  //----------------------------------------------------------------------------

  // PRT-NEXT: {
  {
    // PRT-NEXT: long l = 5;
    long l = 5;
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCVectorClause
    // DMP-NEXT:   ACCReductionClause {{.*}} '+
    // DMP-NEXT:     DeclRefExpr {{.*}} 'l' 'long'
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPSimdDirective
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'l' 'long'
    //      DMP:     CompoundAssignOperator {{.*}} '+='
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop vector reduction(+: l){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp simd reduction(+: l){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp simd reduction(+: l){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop vector reduction(+: l){{$}}
    //    PRT-NEXT: for ({{.*}})
    //    PRT-NEXT:   l += 10;
    #pragma acc loop vector reduction(+: l)
    for (int i = 0; i < 2; ++i)
      l += 10;
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: withinVectorFn: vector: l = 25{{$}}
    // EXE-TGT-USE-STDIO-DAG: withinVectorFn: vector: l = 25{{$}}
    TGT_PRINTF("withinVectorFn: vector: l = %ld\n", l);
  } // PRT-NEXT: }
} // PRT-NEXT: }

//==============================================================================
// Check within seq functions.
//
// This repeats some of the above testing but within a seq function.
//==============================================================================

// DMP-LABEL: FunctionDecl {{.*}} withinSeqFn
// PRT-LABEL: static void withinSeqFn(int param) {
static void withinSeqFn(int param) {
  // PRT-NEXT: int local;
  int local;

  //----------------------------------------------------------------------------
  // Explicit seq.
  //----------------------------------------------------------------------------

  // PRT-NEXT: param = 10;
  // PRT-NEXT: local = 20;
  param = 10;
  local = 20;
  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCSeqClause
  // DMP-NEXT:   ACCReductionClause {{.*}} '*'
  // DMP-NEXT:     DeclRefExpr {{.*}} 'param' 'int'
  // DMP-NEXT:   ACCReductionClause {{.*}} '+'
  // DMP-NEXT:     DeclRefExpr {{.*}} 'local' 'int'
  // DMP-NEXT:   impl: ForStmt
  //      DMP:     CompoundAssignOperator {{.*}} 'int' '*='
  //      DMP:     CompoundAssignOperator {{.*}} 'int' '+='
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop seq reduction(*: param) reduction(+: local)
  // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
  //  PRT-A-SAME: {{^$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop seq reduction(*: param) reduction(+: local) // discarded in OpenMP translation{{$}}
  //    PRT-NEXT: for ({{.*}}) {
  //    PRT-NEXT:   param *= 2;
  //    PRT-NEXT:   local += 2;
  //    PRT-NEXT: }
  #pragma acc loop seq reduction(*: param) reduction(+: local)
  for (int i = 0; i < 2; ++i) {
    param *= 2;
    local += 2;
  }
  // PRT-NEXT: {{TGT_PRINTF|printf}}
  // PRT-NEXT: {{TGT_PRINTF|printf}}
  // EXE-TGT-USE-STDIO-DAG: withinSeqFn: explicit seq: param = 40{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinSeqFn: explicit seq: param = 40{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinSeqFn: explicit seq: local = 24{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinSeqFn: explicit seq: local = 24{{$}}
  TGT_PRINTF("withinSeqFn: explicit seq: param = %d\n", param);
  TGT_PRINTF("withinSeqFn: explicit seq: local = %d\n", local);

  //----------------------------------------------------------------------------
  // Naked loop.
  //----------------------------------------------------------------------------

  // PRT-NEXT: {
  {
    // PRT-NEXT: int arr[] = {10000, 1};
    // PRT-NEXT: int *min = arr;
    // PRT-NEXT: int *max = arr;
    int arr[] = {10000, 1};
    int *min = arr;
    int *max = arr;
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCReductionClause {{.*}} 'min'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'min' 'int *'
    // DMP-NEXT:   ACCReductionClause {{.*}} 'max'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'max' 'int *'
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'arr' 'int[2]'
    // DMP-NEXT:   impl: ForStmt
    //      DMP:       ConditionalOperator
    //      DMP:       ConditionalOperator
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop reduction(min: min) reduction(max: max)
    // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
    //  PRT-A-SAME: {{^$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop reduction(min: min) reduction(max: max) // discarded in OpenMP translation{{$}}
    //    PRT-NEXT: for ({{.*}}) {
    //    PRT-NEXT:   min = arr + i < min ? arr + i : min;
    //    PRT-NEXT:   max = arr + i > max ? arr + i : max;
    //    PRT-NEXT: }
    #pragma acc loop reduction(min: min) reduction(max: max)
    for (int i = 0; i < 2; ++i) {
      min = arr + i < min ? arr + i : min;
      max = arr + i > max ? arr + i : max;
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: withinSeqFn: naked loop: min == arr: 1
    // EXE-TGT-USE-STDIO-DAG: withinSeqFn: naked loop: min == arr: 1
    // EXE-TGT-USE-STDIO-DAG: withinSeqFn: naked loop: max == arr: 0
    // EXE-TGT-USE-STDIO-DAG: withinSeqFn: naked loop: max == arr: 0
    TGT_PRINTF("withinSeqFn: naked loop: min == arr: %d\n", min == arr);
    TGT_PRINTF("withinSeqFn: naked loop: max == arr: %d\n", max == arr);
  } // PRT-NEXT: }

  //----------------------------------------------------------------------------
  // auto.
  //----------------------------------------------------------------------------

  // PRT-NEXT: {
  {
    // PRT-NEXT: _Bool b = 1;
    // PRT-NEXT: param = 10;
    _Bool b = 1;
    param = 10;
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCAutoClause
    // DMP-NEXT:   ACCReductionClause {{.*}} '&'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'b' '_Bool'
    // DMP-NEXT:   ACCReductionClause {{.*}} '|'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'param' 'int'
    // DMP-NEXT:   impl: ForStmt
    //      DMP:     CompoundAssignOperator {{.*}} '_Bool' '&='
    //      DMP:     CompoundAssignOperator {{.*}} 'int' '|='
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop auto reduction(&: b) reduction(|: param)
    // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
    //  PRT-A-SAME: {{^$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop auto reduction(&: b) reduction(|: param) // discarded in OpenMP translation{{$}}
    //    PRT-NEXT: for ({{.*}}) {
    //    PRT-NEXT:   b &= i;
    //    PRT-NEXT:   param |= i;
    //    PRT-NEXT: }
    #pragma acc loop auto reduction(&: b) reduction(|: param)
    for (int i = 0; i < 2; ++i) {
      b &= i;
      param |= i;
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: withinSeqFn: auto with partitioning: b = 0{{$}}
    // EXE-TGT-USE-STDIO-DAG: withinSeqFn: auto with partitioning: b = 0{{$}}
    // EXE-TGT-USE-STDIO-DAG: withinSeqFn: auto with partitioning: param = 11{{$}}
    // EXE-TGT-USE-STDIO-DAG: withinSeqFn: auto with partitioning: param = 11{{$}}
    TGT_PRINTF("withinSeqFn: auto with partitioning: b = %d\n", b);
    TGT_PRINTF("withinSeqFn: auto with partitioning: param = %d\n", param);
  } // PRT-NEXT: }
} // PRT-NEXT: }

// EXE-NOT: {{.}}
