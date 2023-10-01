// Check async clause on "acc parallel" and "acc parallel loop".
//
// async-concurrency.c verifies concurrency among activity queues and host.  See
// its comments for why we don't do so here and in other tests.
//
// REDEFINE: %{all:fc:args} = -DACC_ASYNC_SYNC=-1 -DACC_ASYNC_NOVAL=-2
// REDEFINE: %{dmp:fc:args} = -strict-whitespace
// REDEFINE: %{prt:fc:args} = -match-full-lines
// REDEFINE: %{exe:fc:args} = -strict-whitespace -match-full-lines \
// REDEFINE:                  -implicit-check-not='{{.}}'
//
// FIXME: Skip execution checks when not offloading because we occasionally see
// the following runtime error in that case:
//
//   Assertion failure at kmp_tasking.cpp(4182): task_team != __null.
//   OMP: Error #13: Assertion failure at kmp_tasking.cpp(4182).
//
// RUN: %{acc-check-dmp}
// RUN: %{acc-check-prt}
// RUN: %if host %{%} %else %{ %{acc-check-exe} %}
//
// END.

/* expected-no-diagnostics */

// FIXME: Currently needed for acc2omp_async2dep in OpenMP translation.
#include <openacc.h>
#include <stdio.h>

// We call this to create a boundary for FileCheck directives.
static inline void fcBoundary() {}

int main() {
  //============================================================================
  // Check uncombined "acc parallel" construct in detail.
  //============================================================================

  //----------------------------------------------------------------------------
  // Two asynchronous activity queues via literals.
  //
  // Check that:
  // - Queue 0 kernels run in order: x = 12 not 21 or otherwise.
  // - Queue 1 kernels run in order: y = 12 not 21 or otherwise.
  // - The translation has no handling of the possibility of acc_async_sync as
  //   the compiler can verify that's not possible.
  //----------------------------------------------------------------------------

  // DMP-LABEL:"two async queues via literals\n"
  // PRT-LABEL:{{.*}}"two async queues via literals\n"{{.*}}
  // EXE-LABEL:two async queues via literals
  printf("two async queues via literals\n");
  {
    int x = 0;
    int y = 0;
    #pragma acc enter data copyin(x,y)

    //      DMP: ACCParallelDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 1
    // DMP-NEXT:   ACCPresentClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'x'
    // DMP-NEXT:   ACCAsyncClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:     IntegerLiteral {{.*}} 0
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'x'
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 1
    // DMP-NEXT:     OMPMapClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'x'
    // DMP-NEXT:     OMPNowaitClause
    // DMP-NEXT:     OMPDependClause
    // DMP-NEXT:       UnaryOperator {{.*}} 'char' lvalue prefix '*'
    // DMP-NEXT:         CallExpr {{.*}} 'char *'
    // DMP-NEXT:           ImplicitCastExpr {{.*}} 'char *(*)(int)' <FunctionToPointerDecay>
    // DMP-NEXT:             DeclRefExpr {{.*}} 'acc2omp_async2dep' 'char *(int)'
    // DMP-NEXT:           IntegerLiteral {{.*}} 0
    //  DMP-NOT:     OMP
    //      DMP:     BinaryOperator {{.*}} '='
    //
    //         PRT: fcBoundary();
    //  PRT-A-NEXT: #pragma acc parallel num_gangs(1) present(x) async(0)
    // PRT-AO-NEXT: // #pragma omp target teams num_teams(1) map(present,ompx_hold,alloc: x) nowait depend(inout : *acc2omp_async2dep(0))
    //  PRT-O-NEXT: #pragma omp target teams num_teams(1) map(present,ompx_hold,alloc: x) nowait depend(inout : *acc2omp_async2dep(0))
    // PRT-OA-NEXT: // #pragma acc parallel num_gangs(1) present(x) async(0)
    //    PRT-NEXT: x = x * 10 + 1;
    //    PRT-NEXT: fcBoundary();
    fcBoundary();
    #pragma acc parallel num_gangs(1) present(x) async(0)
    x = x * 10 + 1;
    fcBoundary();

    //      DMP: ACCParallelDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 1
    // DMP-NEXT:   ACCPresentClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'y'
    // DMP-NEXT:   ACCAsyncClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:     IntegerLiteral {{.*}} 1
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'y'
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 1
    // DMP-NEXT:     OMPMapClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'y'
    // DMP-NEXT:     OMPNowaitClause
    // DMP-NEXT:     OMPDependClause
    // DMP-NEXT:       UnaryOperator {{.*}} 'char' lvalue prefix '*'
    // DMP-NEXT:         CallExpr {{.*}} 'char *'
    // DMP-NEXT:           ImplicitCastExpr {{.*}} 'char *(*)(int)' <FunctionToPointerDecay>
    // DMP-NEXT:             DeclRefExpr {{.*}} 'acc2omp_async2dep' 'char *(int)'
    // DMP-NEXT:           IntegerLiteral {{.*}} 1
    //  DMP-NOT:     OMP
    //      DMP:     BinaryOperator {{.*}} '='
    //
    //         PRT: fcBoundary();
    //  PRT-A-NEXT: #pragma acc parallel num_gangs(1) present(y) async(1)
    // PRT-AO-NEXT: // #pragma omp target teams num_teams(1) map(present,ompx_hold,alloc: y) nowait depend(inout : *acc2omp_async2dep(1))
    //  PRT-O-NEXT: #pragma omp target teams num_teams(1) map(present,ompx_hold,alloc: y) nowait depend(inout : *acc2omp_async2dep(1))
    // PRT-OA-NEXT: // #pragma acc parallel num_gangs(1) present(y) async(1)
    //    PRT-NEXT: y = y * 10 + 1;
    //    PRT-NEXT: fcBoundary();
    fcBoundary();
    #pragma acc parallel num_gangs(1) present(y) async(1)
    y = y * 10 + 1;
    fcBoundary();

    #pragma acc parallel num_gangs(1) present(x) async(0)
    x = x * 10 + 2;
    #pragma acc parallel num_gangs(1) present(y) async(1)
    y = y * 10 + 2;

    // EXE-NEXT:two async queues via literals: x=12, y=12
    #pragma acc wait
    #pragma acc exit data copyout(x,y)
    printf("two async queues via literals: x=%d, y=%d\n", x, y);
  }

  //----------------------------------------------------------------------------
  // Two asynchronous activity queues via unsigned variables.
  //
  // Check that:
  // - Queue 0 kernels run in order: x = 12 not 21 or otherwise.
  // - Queue 1 kernels run in order: y = 12 not 21 or otherwise.
  // - The translation has no handling of the possibility of acc_async_sync as
  //   the compiler can verify that's not possible.
  //----------------------------------------------------------------------------

  // DMP-LABEL:"two async queues via unsigned variables\n"
  // PRT-LABEL:{{.*}}"two async queues via unsigned variables\n"{{.*}}
  // EXE-LABEL:two async queues via unsigned variables
  printf("two async queues via unsigned variables\n");
  {
    unsigned qa = 0;
    unsigned qb = 1;
    int x = 0;
    int y = 0;
    #pragma acc enter data copyin(x,y)

    //      DMP: ACCParallelDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 1
    // DMP-NEXT:   ACCPresentClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'x'
    // DMP-NEXT:   ACCAsyncClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'qa'
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'x'
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 1
    // DMP-NEXT:     OMPMapClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'x'
    // DMP-NEXT:     OMPNowaitClause
    // DMP-NEXT:     OMPDependClause
    // DMP-NEXT:       UnaryOperator {{.*}} 'char' lvalue prefix '*'
    // DMP-NEXT:         CallExpr {{.*}} 'char *'
    // DMP-NEXT:           ImplicitCastExpr {{.*}} 'char *(*)(int)' <FunctionToPointerDecay>
    // DMP-NEXT:             DeclRefExpr {{.*}} 'acc2omp_async2dep' 'char *(int)'
    // DMP-NEXT:           ImplicitCastExpr {{.*}} 'int' <IntegralCast>
    // DMP-NEXT:             ImplicitCastExpr {{.*}} 'unsigned int' <LValueToRValue>
    // DMP-NEXT:               DeclRefExpr {{.*}} 'qa'
    //  DMP-NOT:     OMP
    //      DMP:     BinaryOperator {{.*}} '='
    //
    //         PRT: fcBoundary();
    //  PRT-A-NEXT: #pragma acc parallel num_gangs(1) present(x) async(qa)
    // PRT-AO-NEXT: // #pragma omp target teams num_teams(1) map(present,ompx_hold,alloc: x) nowait depend(inout : *acc2omp_async2dep(qa))
    //  PRT-O-NEXT: #pragma omp target teams num_teams(1) map(present,ompx_hold,alloc: x) nowait depend(inout : *acc2omp_async2dep(qa))
    // PRT-OA-NEXT: // #pragma acc parallel num_gangs(1) present(x) async(qa)
    //    PRT-NEXT: x = x * 10 + 1;
    //    PRT-NEXT: fcBoundary();
    fcBoundary();
    #pragma acc parallel num_gangs(1) present(x) async(qa)
    x = x * 10 + 1;
    fcBoundary();

    fcBoundary();
    #pragma acc parallel num_gangs(1) present(y) async(qb)
    y = y * 10 + 1;
    fcBoundary();

    #pragma acc parallel num_gangs(1) present(x) async(qa)
    x = x * 10 + 2;
    #pragma acc parallel num_gangs(1) present(y) async(qb)
    y = y * 10 + 2;

    // EXE-NEXT:two async queues via unsigned variables: x=12, y=12
    #pragma acc wait
    #pragma acc exit data copyout(x,y)
    printf("two async queues via unsigned variables: x=%d, y=%d\n", x, y);
  }

  //----------------------------------------------------------------------------
  // Two asynchronous activity queues via signed variables.
  //
  // Check that:
  // - Queue 0 kernels run in order: x = 12 not 21 or otherwise.
  // - Queue 1 kernels run in order: y = 12 not 21 or otherwise.
  // - The translation has handling of the possibility of acc_async_sync as the
  //   compiler cannot verify that's not possible.
  //----------------------------------------------------------------------------

  // DMP-LABEL:"two async queues via signed variables\n"
  // PRT-LABEL:{{.*}}"two async queues via signed variables\n"{{.*}}
  // EXE-LABEL:two async queues via signed variables
  printf("two async queues via signed variables\n");
  {
    int qa = 3;
    int qb = 8;
    int x = 0;
    int y = 0;
    #pragma acc enter data copyin(x,y)

    //      DMP: ACCParallelDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 1
    // DMP-NEXT:   ACCPresentClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'x'
    // DMP-NEXT:   ACCAsyncClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:     ImplicitCastExpr {{.*}} <LValueToRValue>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'qa'
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'x'
    // DMP-NEXT:   impl: CompoundStmt
    // DMP-NEXT:     OMPTargetTeamsDirective
    // DMP-NEXT:       OMPNum_teamsClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 1
    // DMP-NEXT:       OMPMapClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'x'
    // DMP-NEXT:       OMPNowaitClause
    // DMP-NEXT:       OMPDependClause
    // DMP-NEXT:         UnaryOperator {{.*}} 'char' lvalue prefix '*'
    // DMP-NEXT:           CallExpr {{.*}} 'char *'
    // DMP-NEXT:             ImplicitCastExpr {{.*}} 'char *(*)(int)' <FunctionToPointerDecay>
    // DMP-NEXT:               DeclRefExpr {{.*}} 'acc2omp_async2dep' 'char *(int)'
    // DMP-NEXT:             ImplicitCastExpr
    // DMP-NEXT:               DeclRefExpr {{.*}} 'qa'
    //  DMP-NOT:       OMP
    //      DMP:       BinaryOperator {{.*}} '='
    //  DMP-NOT:     OMP{{.*(Directive|Clause)}}
    //      DMP:     OMPTaskwaitDirective
    // DMP-NEXT:       OMPDependClause
    // DMP-NEXT:         UnaryOperator {{.*}} 'char' lvalue prefix '*'
    // DMP-NEXT:           CallExpr {{.*}} 'char *'
    // DMP-NEXT:             ImplicitCastExpr {{.*}} 'char *(*)(int)' <FunctionToPointerDecay>
    // DMP-NEXT:               DeclRefExpr {{.*}} 'acc2omp_async2dep' 'char *(int)'
    // DMP-NEXT:             IntegerLiteral {{.*}} [[ACC_ASYNC_SYNC]]
    //
    //            PRT: fcBoundary();
    // PRT-NOACC-NEXT: x = x * 10 + 1;
    //    PRT-AO-NEXT: // v----------ACC----------v
    //     PRT-A-NEXT: #pragma acc parallel num_gangs(1) present(x) async(qa)
    //     PRT-A-NEXT: x = x * 10 + 1;
    //    PRT-AO-NEXT: // ---------ACC->OMP--------
    //    PRT-AO-NEXT: // {
    //    PRT-AO-NEXT: //   #pragma omp target teams num_teams(1) map(present,ompx_hold,alloc: x) nowait depend(inout : *acc2omp_async2dep(qa))
    //    PRT-AO-NEXT: //   x = x * 10 + 1;
    //    PRT-AO-NEXT: //   #pragma omp taskwait depend(inout : *acc2omp_async2dep([[ACC_ASYNC_SYNC]]))
    //    PRT-AO-NEXT: // }
    //    PRT-AO-NEXT: // ^----------OMP----------^
    //    PRT-OA-NEXT: // v----------OMP----------v
    //     PRT-O-NEXT: {
    //     PRT-O-NEXT:   #pragma omp target teams num_teams(1) map(present,ompx_hold,alloc: x) nowait depend(inout : *acc2omp_async2dep(qa))
    //     PRT-O-NEXT:   x = x * 10 + 1;
    //     PRT-O-NEXT:   #pragma omp taskwait depend(inout : *acc2omp_async2dep([[ACC_ASYNC_SYNC]]))
    //     PRT-O-NEXT: }
    //    PRT-OA-NEXT: // ---------OMP<-ACC--------
    //    PRT-OA-NEXT: // #pragma acc parallel num_gangs(1) present(x) async(qa)
    //    PRT-OA-NEXT: // x = x * 10 + 1;
    //    PRT-OA-NEXT: // ^----------ACC----------^
    //       PRT-NEXT: fcBoundary();
    fcBoundary();
    #pragma acc parallel num_gangs(1) present(x) async(qa)
    x = x * 10 + 1;
    fcBoundary();

    #pragma acc parallel num_gangs(1) present(y) async(qb)
    y = y * 10 + 1;
    #pragma acc parallel num_gangs(1) present(x) async(qa)
    x = x * 10 + 2;
    #pragma acc parallel num_gangs(1) present(y) async(qb)
    y = y * 10 + 2;

    // EXE-NEXT:two async queues via signed variables: x=12, y=12
    #pragma acc wait
    #pragma acc exit data copyout(x,y)
    printf("two async queues via signed variables: x=%d, y=%d\n", x, y);
  }

  //----------------------------------------------------------------------------
  // Check various ways of specifying the default activity queue when it's an
  // asynchronous activity queue.
  //
  // If the current default queue is selected, the compiler cannot be sure which
  // activity queue that will be at run time, so it must handle the possibility
  // of acc_async_sync.  If the initial default queue is selected, the compiler
  // assumes it's activity queue 0 because the compiler assumes libacc2omp's
  // implementation is being used.
  //----------------------------------------------------------------------------

  // DMP-LABEL:"default queue when async\n"
  // PRT-LABEL:{{.*}}"default queue when async\n"{{.*}}
  // EXE-LABEL:default queue when async
  printf("default queue when async\n");
  {
    int x = 0;
    int y = 0;
    #pragma acc enter data copyin(x,y)

    // Current default activity queue happens to be the initial default activity
    // queue, 0.
    //
    //      DMP: DeclRefExpr {{.*}} 'fcBoundary'
    //      DMP: ACCParallelDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 1
    // DMP-NEXT:   ACCPresentClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'x'
    // DMP-NEXT:   ACCAsyncClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:     <<<NULL>>>
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'x'
    // DMP-NEXT:   impl: CompoundStmt
    // DMP-NEXT:     OMPTargetTeamsDirective
    // DMP-NEXT:       OMPNum_teamsClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 1
    // DMP-NEXT:       OMPMapClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'x'
    // DMP-NEXT:       OMPNowaitClause
    // DMP-NEXT:       OMPDependClause
    // DMP-NEXT:         UnaryOperator {{.*}} 'char' lvalue prefix '*'
    // DMP-NEXT:           CallExpr {{.*}} 'char *'
    // DMP-NEXT:             ImplicitCastExpr {{.*}} 'char *(*)(int)' <FunctionToPointerDecay>
    // DMP-NEXT:               DeclRefExpr {{.*}} 'acc2omp_async2dep' 'char *(int)'
    // DMP-NEXT:             IntegerLiteral {{.*}} [[ACC_ASYNC_NOVAL]]
    //  DMP-NOT:       OMP
    //      DMP:       BinaryOperator {{.*}} '='
    //  DMP-NOT:       OMP{{.*(Directive|Clause)}}
    //      DMP:       OMPTaskwaitDirective
    // DMP-NEXT:         OMPDependClause
    // DMP-NEXT:           UnaryOperator {{.*}} 'char' lvalue prefix '*'
    // DMP-NEXT:             CallExpr {{.*}} 'char *'
    // DMP-NEXT:               ImplicitCastExpr {{.*}} 'char *(*)(int)' <FunctionToPointerDecay>
    // DMP-NEXT:                 DeclRefExpr {{.*}} 'acc2omp_async2dep' 'char *(int)'
    // DMP-NEXT:               IntegerLiteral {{.*}} [[ACC_ASYNC_SYNC]]
    //      DMP: DeclRefExpr {{.*}} 'fcBoundary'
    //
    //            PRT: fcBoundary();
    // PRT-NOACC-NEXT: x = x * 10 + 1;
    //    PRT-AO-NEXT: // v----------ACC----------v
    //     PRT-A-NEXT: #pragma acc parallel num_gangs(1) present(x) async
    //     PRT-A-NEXT: x = x * 10 + 1;
    //    PRT-AO-NEXT: // ---------ACC->OMP--------
    //    PRT-AO-NEXT: // {
    //    PRT-AO-NEXT: //   #pragma omp target teams num_teams(1) map(present,ompx_hold,alloc: x) nowait depend(inout : *acc2omp_async2dep([[ACC_ASYNC_NOVAL]]))
    //    PRT-AO-NEXT: //   x = x * 10 + 1;
    //    PRT-AO-NEXT: //   #pragma omp taskwait depend(inout : *acc2omp_async2dep([[ACC_ASYNC_SYNC]]))
    //    PRT-AO-NEXT: // }
    //    PRT-AO-NEXT: // ^----------OMP----------^
    //    PRT-OA-NEXT: // v----------OMP----------v
    //     PRT-O-NEXT: {
    //     PRT-O-NEXT:   #pragma omp target teams num_teams(1) map(present,ompx_hold,alloc: x) nowait depend(inout : *acc2omp_async2dep([[ACC_ASYNC_NOVAL]]))
    //     PRT-O-NEXT:   x = x * 10 + 1;
    //     PRT-O-NEXT:   #pragma omp taskwait depend(inout : *acc2omp_async2dep([[ACC_ASYNC_SYNC]]))
    //     PRT-O-NEXT: }
    //    PRT-OA-NEXT: // ---------OMP<-ACC--------
    //    PRT-OA-NEXT: // #pragma acc parallel num_gangs(1) present(x) async
    //    PRT-OA-NEXT: // x = x * 10 + 1;
    //    PRT-OA-NEXT: // ^----------ACC----------^
    //       PRT-NEXT: fcBoundary();
    fcBoundary();
    #pragma acc parallel num_gangs(1) present(x) async // queue 0
    x = x * 10 + 1;
    fcBoundary();

    // Just do a quick print check to make sure the translation is basically the
    // same as above.
    //
    // DMP: DeclRefExpr {{.*}} 'fcBoundary'
    //
    // PRT-OA-NEXT: // v----------OMP----------v
    //  PRT-O-NEXT: {
    //  PRT-O-NEXT:   #pragma omp target teams num_teams(1) map(present,ompx_hold,alloc: x) nowait depend(inout : *acc2omp_async2dep(acc_async_noval))
    //  PRT-O-NEXT:   x = x * 10 + 2;
    //  PRT-O-NEXT:   #pragma omp taskwait depend(inout : *acc2omp_async2dep([[ACC_ASYNC_SYNC]]))
    //  PRT-O-NEXT: }
    // PRT-OA-NEXT: // ---------OMP<-ACC--------
    //         PRT: fcBoundary();
    #pragma acc parallel num_gangs(1) present(x) async(acc_async_noval) // queue 0
    x = x * 10 + 2;
    fcBoundary();

    // Initial default activity queue, which libacc2omp guarantees is 0.
    //
    //      DMP: ACCParallelDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 1
    // DMP-NEXT:   ACCPresentClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'x'
    // DMP-NEXT:   ACCAsyncClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'acc_async_default' 'int'
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'x'
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 1
    // DMP-NEXT:     OMPMapClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'x'
    // DMP-NEXT:     OMPNowaitClause
    // DMP-NEXT:     OMPDependClause
    // DMP-NEXT:       UnaryOperator {{.*}} 'char' lvalue prefix '*'
    // DMP-NEXT:         CallExpr {{.*}} 'char *'
    // DMP-NEXT:           ImplicitCastExpr {{.*}} 'char *(*)(int)' <FunctionToPointerDecay>
    // DMP-NEXT:             DeclRefExpr {{.*}} 'acc2omp_async2dep' 'char *(int)'
    // DMP-NEXT:           DeclRefExpr {{.*}} 'acc_async_default'
    //  DMP-NOT:     OMP
    //      DMP:     BinaryOperator {{.*}} '='
    //  DMP-NOT:     OMP{{.*(Directive|Clause)}}
    //      DMP: DeclRefExpr {{.*}} 'fcBoundary'
    //
    //  PRT-A-NEXT: #pragma acc parallel num_gangs(1) present(x) async(acc_async_default)
    // PRT-AO-NEXT: // #pragma omp target teams num_teams(1) map(present,ompx_hold,alloc: x) nowait depend(inout : *acc2omp_async2dep(acc_async_default))
    //  PRT-O-NEXT: #pragma omp target teams num_teams(1) map(present,ompx_hold,alloc: x) nowait depend(inout : *acc2omp_async2dep(acc_async_default))
    // PRT-OA-NEXT: // #pragma acc parallel num_gangs(1) present(x) async(acc_async_default)
    //    PRT-NEXT: x = x * 10 + 3;
    //    PRT-NEXT: fcBoundary();
    #pragma acc parallel num_gangs(1) present(x) async(acc_async_default) // queue 0
    x = x * 10 + 3;
    fcBoundary();

    // This just checks the above are actually on queue 0.  If not, this might
    // execute in parallel with the above.  We previously checked the
    // translation of literal async arguments, so don't repeat those dump and
    // print checks here.
    #pragma acc parallel num_gangs(1) present(x) async(0) // queue 0
    x = x * 10 + 4;

    // Make sure that setting a different default queue causes each of those
    // forms to put work on the correct queue.  We just did dump and print
    // checks for those forms, so don't repeat that.
    acc_set_default_async(1);
    #pragma acc parallel num_gangs(1) present(y) async // queue 1
    y = y * 10 + 1;
    #pragma acc parallel num_gangs(1) present(y) async(acc_async_noval) // queue 1
    y = y * 10 + 2;
    #pragma acc parallel num_gangs(1) present(x) async(acc_async_default) // queue 0
    x = x * 10 + 5;
    #pragma acc parallel num_gangs(1) present(y) async(1) // queue 1
    y = y * 10 + 3;

    // EXE-NEXT:default queue when async: x=12345, y=123
    #pragma acc wait
    #pragma acc exit data copyout(x,y)
    printf("default queue when async: x=%d, y=%d\n", x, y);

    acc_set_default_async(acc_async_default);
  }

  //----------------------------------------------------------------------------
  // Synchronous activity queue via literals.
  //
  // Check that:
  // - Kernels run in order with host code: x = 1234567 not 7654321 or
  //   otherwise.
  // - The translation has no handling of the possibility of asynchronous
  //   execution because the compiler can verify that's not possible.
  //----------------------------------------------------------------------------

  // DMP-LABEL:"sync queue via literals\n"
  // PRT-LABEL:{{.*}}"sync queue via literals\n"{{.*}}
  // EXE-LABEL:sync queue via literals
  printf("sync queue via literals\n");
  {
    int x = 0;
    #pragma acc enter data copyin(x)

    x = x * 10 + 1;
    #pragma acc update device(x)

    //      DMP: ACCParallelDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 1
    // DMP-NEXT:   ACCPresentClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'x'
    // DMP-NEXT:   ACCAsyncClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'acc_async_sync'
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'x'
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 1
    // DMP-NEXT:     OMPMapClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'x'
    //  DMP-NOT:     OMP
    //      DMP:     BinaryOperator {{.*}} '='
    //
    //         PRT: fcBoundary();
    //  PRT-A-NEXT: #pragma acc parallel num_gangs(1) present(x) async(acc_async_sync)
    // PRT-AO-NEXT: // #pragma omp target teams num_teams(1) map(present,ompx_hold,alloc: x)
    //  PRT-O-NEXT: #pragma omp target teams num_teams(1) map(present,ompx_hold,alloc: x)
    // PRT-OA-NEXT: // #pragma acc parallel num_gangs(1) present(x) async(acc_async_sync)
    //    PRT-NEXT: x = x * 10 + 2;
    //    PRT-NEXT: fcBoundary();
    fcBoundary();
    #pragma acc parallel num_gangs(1) present(x) async(acc_async_sync)
    x = x * 10 + 2;
    fcBoundary();
    #pragma acc update self(x)

    x = x * 10 + 3;
    #pragma acc update device(x)

    #pragma acc parallel num_gangs(1) present(x) async(acc_async_sync)
    x = x * 10 + 4;
    #pragma acc update self(x)

    x = x * 10 + 5;
    #pragma acc update device(x)

    #pragma acc parallel num_gangs(1) present(x)
    x = x * 10 + 6;
    #pragma acc update self(x)

    x = x * 10 + 7;
    #pragma acc update device(x)

    // EXE-NEXT:sync queue via literals: x=1234567
    #pragma acc exit data copyout(x)
    printf("sync queue via literals: x=%d\n", x);
  }

  //----------------------------------------------------------------------------
  // Synchronous activity queue via variables.
  //
  // Check that:
  // - Kernels run in order with host code: x = 1234567 not 7654321 or
  //   otherwise.
  // - The translation has handling of the possibility of asynchronous execution
  //   because the compiler cannot verify that's not possible.
  //----------------------------------------------------------------------------

  // DMP-LABEL:"sync queue via variables\n"
  // PRT-LABEL:{{.*}}"sync queue via variables\n"{{.*}}
  // EXE-LABEL:sync queue via variables
  printf("sync queue via variables\n");
  {
    int x = 0;
    int qa = acc_async_sync;
    int qb = acc_async_sync;
    #pragma acc enter data copyin(x)

    x = x * 10 + 1;
    #pragma acc update device(x)

    //      DMP: ACCParallelDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 1
    // DMP-NEXT:   ACCPresentClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'x'
    // DMP-NEXT:   ACCAsyncClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:     ImplicitCastExpr {{.*}} <LValueToRValue>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'qa'
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'x'
    // DMP-NEXT:   impl: CompoundStmt
    // DMP-NEXT:     OMPTargetTeamsDirective
    // DMP-NEXT:       OMPNum_teamsClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 1
    // DMP-NEXT:       OMPMapClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'x'
    // DMP-NEXT:       OMPNowaitClause
    // DMP-NEXT:       OMPDependClause
    // DMP-NEXT:         UnaryOperator {{.*}} 'char' lvalue prefix '*'
    // DMP-NEXT:           CallExpr {{.*}} 'char *'
    // DMP-NEXT:             ImplicitCastExpr {{.*}} 'char *(*)(int)' <FunctionToPointerDecay>
    // DMP-NEXT:               DeclRefExpr {{.*}} 'acc2omp_async2dep' 'char *(int)'
    // DMP-NEXT:             ImplicitCastExpr
    // DMP-NEXT:               DeclRefExpr {{.*}} 'qa'
    //  DMP-NOT:       OMP
    //      DMP:       BinaryOperator {{.*}} '='
    //  DMP-NOT:     OMP{{.*(Directive|Clause)}}
    //      DMP:     OMPTaskwaitDirective
    // DMP-NEXT:       OMPDependClause
    // DMP-NEXT:         UnaryOperator {{.*}} 'char' lvalue prefix '*'
    // DMP-NEXT:           CallExpr {{.*}} 'char *'
    // DMP-NEXT:             ImplicitCastExpr {{.*}} 'char *(*)(int)' <FunctionToPointerDecay>
    // DMP-NEXT:               DeclRefExpr {{.*}} 'acc2omp_async2dep' 'char *(int)'
    // DMP-NEXT:             IntegerLiteral {{.*}} [[ACC_ASYNC_SYNC]]
    //
    //            PRT: fcBoundary();
    // PRT-NOACC-NEXT: x = x * 10 + 2;
    //    PRT-AO-NEXT: // v----------ACC----------v
    //     PRT-A-NEXT: #pragma acc parallel num_gangs(1) present(x) async(qa)
    //     PRT-A-NEXT: x = x * 10 + 2;
    //    PRT-AO-NEXT: // ---------ACC->OMP--------
    //    PRT-AO-NEXT: // {
    //    PRT-AO-NEXT: //   #pragma omp target teams num_teams(1) map(present,ompx_hold,alloc: x) nowait depend(inout : *acc2omp_async2dep(qa))
    //    PRT-AO-NEXT: //   x = x * 10 + 2;
    //    PRT-AO-NEXT: //   #pragma omp taskwait depend(inout : *acc2omp_async2dep([[ACC_ASYNC_SYNC]]))
    //    PRT-AO-NEXT: // }
    //    PRT-AO-NEXT: // ^----------OMP----------^
    //    PRT-OA-NEXT: // v----------OMP----------v
    //     PRT-O-NEXT: {
    //     PRT-O-NEXT:   #pragma omp target teams num_teams(1) map(present,ompx_hold,alloc: x) nowait depend(inout : *acc2omp_async2dep(qa))
    //     PRT-O-NEXT:   x = x * 10 + 2;
    //     PRT-O-NEXT:   #pragma omp taskwait depend(inout : *acc2omp_async2dep([[ACC_ASYNC_SYNC]]))
    //     PRT-O-NEXT: }
    //    PRT-OA-NEXT: // ---------OMP<-ACC--------
    //    PRT-OA-NEXT: // #pragma acc parallel num_gangs(1) present(x) async(qa)
    //    PRT-OA-NEXT: // x = x * 10 + 2;
    //    PRT-OA-NEXT: // ^----------ACC----------^
    //       PRT-NEXT: fcBoundary();
    fcBoundary();
    #pragma acc parallel num_gangs(1) present(x) async(qa)
    x = x * 10 + 2;
    fcBoundary();
    #pragma acc update self(x)

    x = x * 10 + 3;
    #pragma acc update device(x)

    #pragma acc parallel num_gangs(1) present(x) async(qb)
    x = x * 10 + 4;
    #pragma acc update self(x)

    x = x * 10 + 5;
    #pragma acc update device(x)

    #pragma acc parallel num_gangs(1) present(x)
    x = x * 10 + 6;
    #pragma acc update self(x)

    x = x * 10 + 7;
    #pragma acc update device(x)

    // EXE-NEXT:sync queue via variables: x=1234567
    #pragma acc exit data copyout(x)
    printf("sync queue via variables: x=%d\n", x);
  }

  //----------------------------------------------------------------------------
  // Check various ways of specifying the default activity queue when it's the
  // synchronous activity queue.
  //
  // We performed dump and print checks of these forms above.  The difference
  // here is just the activity queue selected at run time, so execution checks
  // are enough.
  //----------------------------------------------------------------------------

  // DMP-LABEL:"default queue when sync\n"
  // PRT-LABEL:{{.*}}"default queue when sync\n"{{.*}}
  // EXE-LABEL:default queue when sync
  printf("default queue when sync\n");
  {
    acc_set_default_async(acc_async_sync);

    int x = 0;
    #pragma acc enter data copyin(x)

    x = x * 10 + 1;
    #pragma acc update device(x)

    #pragma acc parallel num_gangs(1) present(x) async
    x = x * 10 + 2;
    #pragma acc update self(x)

    x = x * 10 + 3;
    #pragma acc update device(x)

    #pragma acc parallel num_gangs(1) present(x) async(acc_async_noval)
    x = x * 10 + 4;
    #pragma acc update self(x)

    x = x * 10 + 5;
    #pragma acc update device(x)

    #pragma acc parallel num_gangs(1) present(x) async(acc_async_sync)
    x = x * 10 + 6;
    #pragma acc update self(x)

    x = x * 10 + 7;
    #pragma acc update device(x)

    // EXE-NEXT:default queue when sync: x=1234567
    #pragma acc exit data copyout(x)
    printf("default queue when sync: x=%d\n", x);

    acc_set_default_async(acc_async_default);
  }

  //============================================================================
  // Check combined "acc parallel loop" construct for just a few basic cases.
  //
  // We assume the remaining cases above would similarly extend from
  // "acc parallel" to "acc parallel loop"
  //============================================================================

  //----------------------------------------------------------------------------
  // Two activity queues known at compile time to be asynchronous.
  //
  // Check that:
  // - Queue 0 kernels run in order: x[i] = 12 not 21 or otherwise.
  // - Queue 1 kernels run in order: y[i] = 12 not 21 or otherwise.
  // - The translation has no handling of the possibility of acc_async_sync as
  //   the compiler can verify that's not possible.
  //----------------------------------------------------------------------------

  // DMP-LABEL:"combined parallel loop, definitely async\n"
  // PRT-LABEL:{{.*}}"combined parallel loop, definitely async\n"{{.*}}
  // EXE-LABEL:combined parallel loop, definitely async
  printf("combined parallel loop, definitely async\n");
  {
    int x[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    int y[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    #pragma acc enter data copyin(x, y)

    //      DMP: ACCParallelLoopDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 8
    // DMP-NEXT:   ACCGangClause
    // DMP-NEXT:   ACCPresentClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'x'
    // DMP-NEXT:   ACCAsyncClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:     IntegerLiteral {{.*}} 0
    // DMP-NEXT:   effect: ACCParallelDirective
    // DMP-NEXT:     ACCNumGangsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 8
    // DMP-NEXT:     ACCPresentClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'x'
    // DMP-NEXT:     ACCAsyncClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       IntegerLiteral {{.*}} 0
    // DMP-NEXT:     ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'x'
    // DMP-NEXT:     impl: OMPTargetTeamsDirective
    // DMP-NEXT:       OMPNum_teamsClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 8
    // DMP-NEXT:       OMPMapClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'x'
    // DMP-NEXT:       OMPNowaitClause
    // DMP-NEXT:       OMPDependClause
    // DMP-NEXT:         UnaryOperator {{.*}} 'char' lvalue prefix '*'
    // DMP-NEXT:           CallExpr {{.*}} 'char *'
    // DMP-NEXT:             ImplicitCastExpr {{.*}} 'char *(*)(int)' <FunctionToPointerDecay>
    // DMP-NEXT:               DeclRefExpr {{.*}} 'acc2omp_async2dep' 'char *(int)'
    // DMP-NEXT:             IntegerLiteral {{.*}} 0
    //  DMP-NOT:       OMP{{.*}}Clause
    //      DMP:     ACCLoopDirective
    // DMP-NEXT:       ACCGangClause
    // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:       ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:         DeclRefExpr {{.*}} 'x'
    // DMP-NEXT:       impl: OMPDistributeDirective
    //  DMP-NOT:         OMP
    //      DMP:         ForStmt
    //
    //         PRT: fcBoundary();
    //  PRT-A-NEXT: #pragma acc parallel loop num_gangs(8) gang present(x) async(0)
    // PRT-AO-NEXT: // #pragma omp target teams num_teams(8) map(present,ompx_hold,alloc: x) nowait depend(inout : *acc2omp_async2dep(0))
    // PRT-AO-NEXT: // #pragma omp distribute
    //  PRT-O-NEXT: #pragma omp target teams num_teams(8) map(present,ompx_hold,alloc: x) nowait depend(inout : *acc2omp_async2dep(0))
    //  PRT-O-NEXT: #pragma omp distribute
    // PRT-OA-NEXT: // #pragma acc parallel loop num_gangs(8) gang present(x) async(0)
    //    PRT-NEXT: for (int i = 0; i < 8; ++i)
    //    PRT-NEXT:   x[i] = x[i] * 10 + 1;
    //    PRT-NEXT: fcBoundary();
    fcBoundary();
    #pragma acc parallel loop num_gangs(8) gang present(x) async(0)
    for (int i = 0; i < 8; ++i)
      x[i] = x[i] * 10 + 1;
    fcBoundary();

    // Don't repeat the dump and print checks from above as the form here is the
    // same and we previously performed dump and print checks for a different
    // literal activity queue number when checking uncombined "acc parallel".
    #pragma acc parallel loop num_gangs(1) gang present(y) async(1)
    for (int i = 0; i < 8; ++i)
      y[i] = y[i] * 10 + 1;
    #pragma acc parallel loop num_gangs(1) gang present(x) async(0)
    for (int i = 0; i < 8; ++i)
      x[i] = x[i] * 10 + 2;
    #pragma acc parallel loop num_gangs(1) gang present(y) async(1)
    for (int i = 0; i < 8; ++i)
      y[i] = y[i] * 10 + 2;

    // EXE-NEXT:combined parallel loop, definitely async:
    // EXE-NEXT:  x[0]=12, y[0]=12
    // EXE-NEXT:  x[1]=12, y[1]=12
    // EXE-NEXT:  x[2]=12, y[2]=12
    // EXE-NEXT:  x[3]=12, y[3]=12
    // EXE-NEXT:  x[4]=12, y[4]=12
    // EXE-NEXT:  x[5]=12, y[5]=12
    // EXE-NEXT:  x[6]=12, y[6]=12
    // EXE-NEXT:  x[7]=12, y[7]=12
    #pragma acc wait
    #pragma acc exit data copyout(x,y)
    printf("combined parallel loop, definitely async:\n");
    for (int i = 0; i < 8; ++i)
      printf("  x[%d]=%d, y[%d]=%d\n", i, x[i], i, y[i]);
  }

  //----------------------------------------------------------------------------
  // Activity queue known at compile time to be synchronous.
  //
  // Check that:
  // - Kernels run in order with host code: x = 1234567 not 7654321 or
  //   otherwise.
  // - The translation has no handling of the possibility of asynchronous
  //   execution because the compiler can verify that's not possible.
  //----------------------------------------------------------------------------

  // DMP-LABEL:"combined parallel loop, definitely sync\n"
  // PRT-LABEL:{{.*}}"combined parallel loop, definitely sync\n"{{.*}}
  // EXE-LABEL:combined parallel loop, definitely sync
  printf("combined parallel loop, definitely sync\n");
  {
    int x[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    #pragma acc enter data copyin(x)

    for (int i = 0; i < 8; ++i)
      x[i] = x[i] * 10 + 1;
    #pragma acc update device(x)

    //      DMP: ACCParallelLoopDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 8
    // DMP-NEXT:   ACCGangClause
    // DMP-NEXT:   ACCPresentClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'x'
    // DMP-NEXT:   ACCAsyncClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'acc_async_sync'
    // DMP-NEXT:   effect: ACCParallelDirective
    // DMP-NEXT:     ACCNumGangsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 8
    // DMP-NEXT:     ACCPresentClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'x'
    // DMP-NEXT:     ACCAsyncClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'acc_async_sync'
    // DMP-NEXT:     ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'x'
    // DMP-NEXT:     impl: OMPTargetTeamsDirective
    // DMP-NEXT:       OMPNum_teamsClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 8
    // DMP-NEXT:       OMPMapClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'x'
    //  DMP-NOT:       OMP{{.*}}Clause
    //      DMP:     ACCLoopDirective
    // DMP-NEXT:       ACCGangClause
    // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:       ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:         DeclRefExpr {{.*}} 'x'
    // DMP-NEXT:       impl: OMPDistributeDirective
    //  DMP-NOT:         OMP
    //      DMP:         ForStmt
    //
    //         PRT: fcBoundary();
    //  PRT-A-NEXT: #pragma acc parallel loop num_gangs(8) gang present(x) async(acc_async_sync)
    // PRT-AO-NEXT: // #pragma omp target teams num_teams(8) map(present,ompx_hold,alloc: x)
    // PRT-AO-NEXT: // #pragma omp distribute
    //  PRT-O-NEXT: #pragma omp target teams num_teams(8) map(present,ompx_hold,alloc: x)
    //  PRT-O-NEXT: #pragma omp distribute
    // PRT-OA-NEXT: // #pragma acc parallel loop num_gangs(8) gang present(x) async(acc_async_sync)
    //    PRT-NEXT: for (int i = 0; i < 8; ++i)
    //    PRT-NEXT:   x[i] = x[i] * 10 + 2;
    //    PRT-NEXT: fcBoundary();
    fcBoundary();
    #pragma acc parallel loop num_gangs(8) gang present(x) async(acc_async_sync)
    for (int i = 0; i < 8; ++i)
      x[i] = x[i] * 10 + 2;
    fcBoundary();
    #pragma acc update self(x)

    for (int i = 0; i < 8; ++i)
      x[i] = x[i] * 10 + 3;
    #pragma acc update device(x)

    #pragma acc parallel loop num_gangs(8) gang present(x) async(acc_async_sync)
    for (int i = 0; i < 8; ++i)
      x[i] = x[i] * 10 + 4;
    #pragma acc update self(x)

    for (int i = 0; i < 8; ++i)
      x[i] = x[i] * 10 + 5;
    #pragma acc update device(x)

    #pragma acc parallel loop num_gangs(8) gang present(x) async(acc_async_sync)
    for (int i = 0; i < 8; ++i)
      x[i] = x[i] * 10 + 6;
    #pragma acc update self(x)

    for (int i = 0; i < 8; ++i)
      x[i] = x[i] * 10 + 7;
    #pragma acc update device(x)

    // EXE-NEXT:combined parallel loop, definitely sync:
    // EXE-NEXT:  x[0]=1234567
    // EXE-NEXT:  x[1]=1234567
    // EXE-NEXT:  x[2]=1234567
    // EXE-NEXT:  x[3]=1234567
    // EXE-NEXT:  x[4]=1234567
    // EXE-NEXT:  x[5]=1234567
    // EXE-NEXT:  x[6]=1234567
    // EXE-NEXT:  x[7]=1234567
    #pragma acc exit data copyout(x)
    printf("combined parallel loop, definitely sync:\n");
    for (int i = 0; i < 8; ++i)
      printf("  x[%d]=%d\n", i, x[i]);
  }

  //----------------------------------------------------------------------------
  // Two activity queues that might be synchronous or asynchronous at run time.
  //
  // Check that:
  // - Queue 0 kernels run in order: x = 12 not 21 or otherwise.
  // - Queue 1 kernels run in order: y = 12 not 21 or otherwise.
  // - The translation has handling of the possibility of acc_async_sync as the
  //   compiler cannot verify that's not possible.
  //----------------------------------------------------------------------------

  // DMP-LABEL:"combined parallel loop, maybe sync or async\n"
  // PRT-LABEL:{{.*}}"combined parallel loop, maybe sync or async\n"{{.*}}
  // EXE-LABEL:combined parallel loop, maybe sync or async
  printf("combined parallel loop, maybe sync or async\n");
  {
    int qa = 3;
    int qb = 8;
    int x[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    int y[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    #pragma acc enter data copyin(x,y)

    //      DMP: ACCParallelLoopDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 8
    // DMP-NEXT:   ACCGangClause
    // DMP-NEXT:   ACCPresentClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'x'
    // DMP-NEXT:   ACCAsyncClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:     ImplicitCastExpr {{.*}} <LValueToRValue>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'qa'
    // DMP-NEXT:   effect: ACCParallelDirective
    // DMP-NEXT:     ACCNumGangsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 8
    // DMP-NEXT:     ACCPresentClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'x'
    // DMP-NEXT:     ACCAsyncClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       ImplicitCastExpr {{.*}} <LValueToRValue>
    // DMP-NEXT:         DeclRefExpr {{.*}} 'qa'
    // DMP-NEXT:     ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'x'
    // DMP-NEXT:     impl: CompoundStmt
    // DMP-NEXT:       OMPTargetTeamsDirective
    // DMP-NEXT:         OMPNum_teamsClause
    // DMP-NEXT:           IntegerLiteral {{.*}} 8
    // DMP-NEXT:         OMPMapClause
    // DMP-NEXT:           DeclRefExpr {{.*}} 'x'
    // DMP-NEXT:         OMPNowaitClause
    // DMP-NEXT:         OMPDependClause
    // DMP-NEXT:           UnaryOperator {{.*}} 'char' lvalue prefix '*'
    // DMP-NEXT:             CallExpr {{.*}} 'char *'
    // DMP-NEXT:               ImplicitCastExpr {{.*}} 'char *(*)(int)' <FunctionToPointerDecay>
    // DMP-NEXT:                 DeclRefExpr {{.*}} 'acc2omp_async2dep' 'char *(int)'
    // DMP-NEXT:               ImplicitCastExpr
    // DMP-NEXT:                 DeclRefExpr {{.*}} 'qa'
    //  DMP-NOT:         OMP
    //      DMP:         OMPDistributeDirective
    //  DMP-NOT:           OMP
    //      DMP:           ForStmt
    //  DMP-NOT:       OMP{{.*Clause}}
    //      DMP:       OMPTaskwaitDirective
    // DMP-NEXT:         OMPDependClause
    // DMP-NEXT:           UnaryOperator {{.*}} 'char' lvalue prefix '*'
    // DMP-NEXT:             CallExpr {{.*}} 'char *'
    // DMP-NEXT:               ImplicitCastExpr {{.*}} 'char *(*)(int)' <FunctionToPointerDecay>
    // DMP-NEXT:                 DeclRefExpr {{.*}} 'acc2omp_async2dep' 'char *(int)'
    // DMP-NEXT:               IntegerLiteral {{.*}} [[ACC_ASYNC_SYNC]]
    //      DMP:     ACCLoopDirective
    // DMP-NEXT:       ACCGangClause
    // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:       ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:         DeclRefExpr {{.*}} 'x'
    // DMP-NEXT:       impl: OMPDistributeDirective
    //  DMP-NOT:         OMP
    //      DMP:         ForStmt
    //
    //            PRT: fcBoundary();
    // PRT-NOACC-NEXT: for (int i = 0; i < 8; ++i)
    // PRT-NOACC-NEXT:   x[i] = x[i] * 10 + 1;
    //    PRT-AO-NEXT: // v----------ACC----------v
    //     PRT-A-NEXT: #pragma acc parallel loop num_gangs(8) gang present(x) async(qa)
    //     PRT-A-NEXT: for (int i = 0; i < 8; ++i)
    //     PRT-A-NEXT:   x[i] = x[i] * 10 + 1;
    //    PRT-AO-NEXT: // ---------ACC->OMP--------
    //    PRT-AO-NEXT: // {
    //    PRT-AO-NEXT: //   #pragma omp target teams num_teams(8) map(present,ompx_hold,alloc: x) nowait depend(inout : *acc2omp_async2dep(qa))
    //    PRT-AO-NEXT: //   #pragma omp distribute
    //    PRT-AO-NEXT: //   for (int i = 0; i < 8; ++i)
    //    PRT-AO-NEXT: //     x[i] = x[i] * 10 + 1;
    //    PRT-AO-NEXT: //   #pragma omp taskwait depend(inout : *acc2omp_async2dep([[ACC_ASYNC_SYNC]]))
    //    PRT-AO-NEXT: // }
    //    PRT-AO-NEXT: // ^----------OMP----------^
    //    PRT-OA-NEXT: // v----------OMP----------v
    //     PRT-O-NEXT: {
    //     PRT-O-NEXT:   #pragma omp target teams num_teams(8) map(present,ompx_hold,alloc: x) nowait depend(inout : *acc2omp_async2dep(qa))
    //     PRT-O-NEXT:   #pragma omp distribute
    //     PRT-O-NEXT:   for (int i = 0; i < 8; ++i)
    //     PRT-O-NEXT:     x[i] = x[i] * 10 + 1;
    //     PRT-O-NEXT:   #pragma omp taskwait depend(inout : *acc2omp_async2dep([[ACC_ASYNC_SYNC]]))
    //     PRT-O-NEXT: }
    //    PRT-OA-NEXT: // ---------OMP<-ACC--------
    //    PRT-OA-NEXT: // #pragma acc parallel loop num_gangs(8) gang present(x) async(qa)
    //    PRT-OA-NEXT: // for (int i = 0; i < 8; ++i)
    //    PRT-OA-NEXT: //   x[i] = x[i] * 10 + 1;
    //    PRT-OA-NEXT: // ^----------ACC----------^
    //       PRT-NEXT: fcBoundary();
    fcBoundary();
    #pragma acc parallel loop num_gangs(8) gang present(x) async(qa)
    for (int i = 0; i < 8; ++i)
      x[i] = x[i] * 10 + 1;
    fcBoundary();

    #pragma acc parallel loop num_gangs(1) gang present(y) async(qb)
    for (int i = 0; i < 8; ++i)
      y[i] = y[i] * 10 + 1;
    #pragma acc parallel loop num_gangs(1) gang present(x) async(qa)
    for (int i = 0; i < 8; ++i)
      x[i] = x[i] * 10 + 2;
    #pragma acc parallel loop num_gangs(1) gang present(y) async(qb)
    for (int i = 0; i < 8; ++i)
      y[i] = y[i] * 10 + 2;

    // EXE-NEXT:combined parallel loop, maybe sync or async:
    // EXE-NEXT:  x[0]=12, y[0]=12
    // EXE-NEXT:  x[1]=12, y[1]=12
    // EXE-NEXT:  x[2]=12, y[2]=12
    // EXE-NEXT:  x[3]=12, y[3]=12
    // EXE-NEXT:  x[4]=12, y[4]=12
    // EXE-NEXT:  x[5]=12, y[5]=12
    // EXE-NEXT:  x[6]=12, y[6]=12
    // EXE-NEXT:  x[7]=12, y[7]=12
    #pragma acc wait
    #pragma acc exit data copyout(x,y)
    printf("combined parallel loop, maybe sync or async:\n");
    for (int i = 0; i < 8; ++i)
      printf("  x[%d]=%d, y[%d]=%d\n", i, x[i], i, y[i]);
  }

  return 0;
}
