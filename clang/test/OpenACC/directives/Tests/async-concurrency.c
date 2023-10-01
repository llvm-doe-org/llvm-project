// Check that our scheme for translating the async clause to OpenMP correctly
// specifies concurrency among activity queues and the host.
//
// Verifying concurrency requires questionable attempts in application code to
// communicate among activity queues and the host, and verification failure
// means deadlock during execution checks.  Thus, to facilitate maintenance of
// the test suite, we limit such concurrency verification to a small number of
// usage scenarios in this test file only.  Other test files (e.g.,
// parallel-async.c) cover many other usage scenarios and verify that each
// activity queue executes in order, but they don't verify concurrency.  It's up
// to dump and print checks everywhere to verify that the same OpenMP
// translation scheme is employed for all usage scenarios.
//
// FIXME: Currently, the aforementioned deadlock occurs sometimes in this test
// (see FIXME below), so we specify ALLOW_RETRIES.  Again, the goal of this test
// is to check that our translation to OpenMP correctly specifies concurrency
// among activity queues and the host.  If we sometimes see that concurrency and
// sometimes do not, it is our bet that our translation is correct but that the
// OpenMP runtime has a nondeterminism.  The latter is for the OpenMP developers
// and tests to sort out.
//
// ALLOW_RETRIES: 5
//
// FIXME: Skip execution checks when not offloading because we occasionally see
// the following runtime error in that case:
//
//   Assertion failure at kmp_tasking.cpp(4182): task_team != __null.
//   OMP: Error #13: Assertion failure at kmp_tasking.cpp(4182).
//
// REDEFINE: %{all:fc:args-stable} = -DACC_ASYNC_SYNC=-1
// REDEFINE: %{dmp:fc:args-stable} = -strict-whitespace
// REDEFINE: %{prt:fc:args-stable} = -match-full-lines
// REDEFINE: %{exe:fc:args-stable} = -strict-whitespace -match-full-lines \
// REDEFINE:                         -implicit-check-not='{{.}}'
//
// Check once when the translation has no handling of the possibility of
// acc_async_sync as the compiler can verify that's not possible.
//
// REDEFINE: %{all:clang:args} = -DQ0=0 -DQ1=1
// REDEFINE: %{all:fc:pres} = LIT
// RUN: %{acc-check-dmp}
// RUN: %{acc-check-prt}
// RUN: %if host %{%} %else %{ %{acc-check-exe} %}
//
// Check once when the translation has handling of the possibility of
// acc_async_sync as the compiler cannot verify that's not possible.
//
// REDEFINE: %{all:clang:args} = -DQ0=q0 -DQ1=q1
// REDEFINE: %{all:fc:pres} = VAR
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
  //----------------------------------------------------------------------------
  // Two activity queues and host.
  //
  // Check that:
  // - Queue 0 kernels run in order: x = 12 not 21 or otherwise.
  // - Queue 1 kernels run in order: y = 12 not 21 or otherwise.
  // - Queue 0 and 1 kernels run in parallel with host kernels: no deadlock.
  // - Queue 0 kernels run in parallel with queue 1 kernels: no deadlock.
  //
  // FIXME: The last check listed above occasionally fails (causes the test to
  // hang), whether offloading (reproduced at least for nvptx64, x86_64, and
  // ppc64le) or targeting host, so we set ALLOW_RETRIES above.  At the time
  // of this writing, we could reproduce that hang with upstream Clang and an
  // OpenMP translation of the kernels below.  The first two kernels alone,
  // without the then unnecessary 'depend' clauses, and without the host 'while'
  // loops were sufficient to reproduce.  Adding printf statements to those
  // kernels or attaching a debugger to the process when it hangs seemed to
  // reveal that, in such cases, only one kernel managed to start executing, and
  // not always the same kernel.  It seems there's a nondeterminism in the
  // OpenMP runtime that prevents it from executing other kernels concurrently.
  //----------------------------------------------------------------------------

  // DMP-LABEL:"two queues and host\n"
  // PRT-LABEL:{{.*}}"two queues and host\n"{{.*}}
  // EXE-LABEL:two queues and host
  printf("two queues and host\n");
  {
    int q0 = 0;
    int q1 = 1;
    int x = 0;
    int y = 0;
    int z = 0;
    #pragma acc enter data copyin(x,y,z)

    //          DMP: ACCParallelDirective
    //     DMP-NEXT:   ACCNumGangsClause
    //     DMP-NEXT:     IntegerLiteral {{.*}} 1
    //     DMP-NEXT:   ACCPresentClause
    //     DMP-NEXT:     DeclRefExpr {{.*}} 'x'
    //     DMP-NEXT:     DeclRefExpr {{.*}} 'y'
    //     DMP-NEXT:     DeclRefExpr {{.*}} 'z'
    //     DMP-NEXT:   ACCAsyncClause
    //      DMP-NOT:     <implicit>
    // DMP-LIT-NEXT:     IntegerLiteral {{.*}} 0
    // DMP-VAR-NEXT:     ImplicitCastExpr {{.*}} <LValueToRValue>
    // DMP-VAR-NEXT:       DeclRefExpr {{.*}} 'q0'
    //     DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    //     DMP-NEXT:     DeclRefExpr {{.*}} 'z'
    //     DMP-NEXT:     DeclRefExpr {{.*}} 'y'
    //     DMP-NEXT:     DeclRefExpr {{.*}} 'x'
    // DMP-LIT-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-VAR-NEXT:   impl: CompoundStmt
    // DMP-VAR-NEXT:     OMPTargetTeamsDirective
    //     DMP-NEXT:       OMPNum_teamsClause
    //     DMP-NEXT:         IntegerLiteral {{.*}} 1
    //     DMP-NEXT:       OMPMapClause
    //     DMP-NEXT:         DeclRefExpr {{.*}} 'x'
    //     DMP-NEXT:         DeclRefExpr {{.*}} 'y'
    //     DMP-NEXT:         DeclRefExpr {{.*}} 'z'
    //     DMP-NEXT:       OMPNowaitClause
    //     DMP-NEXT:       OMPDependClause
    //     DMP-NEXT:         UnaryOperator {{.*}} 'char' lvalue prefix '*'
    //     DMP-NEXT:           CallExpr {{.*}} 'char *'
    //     DMP-NEXT:             ImplicitCastExpr {{.*}} 'char *(*)(int)' <FunctionToPointerDecay>
    //     DMP-NEXT:               DeclRefExpr {{.*}} 'acc2omp_async2dep' 'char *(int)'
    // DMP-LIT-NEXT:             IntegerLiteral {{.*}} 0
    // DMP-VAR-NEXT:             ImplicitCastExpr
    // DMP-VAR-NEXT:               DeclRefExpr {{.*}} 'q0'
    //      DMP-NOT:       OMP
    //          DMP:       CompoundStmt
    //
    //             PRT: fcBoundary();

    //     PRT-A-LIT-NEXT: #pragma acc parallel num_gangs(1) present(x,y,z) async({{Q0|0}})
    //    PRT-AO-LIT-NEXT: // #pragma omp target teams num_teams(1) map(present,ompx_hold,alloc: x,y,z) nowait depend(inout : *acc2omp_async2dep(0))
    //     PRT-O-LIT-NEXT: #pragma omp target teams num_teams(1) map(present,ompx_hold,alloc: x,y,z) nowait depend(inout : *acc2omp_async2dep(0))
    //    PRT-OA-LIT-NEXT: // #pragma acc parallel num_gangs(1) present(x,y,z) async({{Q0|0}})
    //       PRT-LIT-NEXT: {
    //       PRT-LIT-NEXT:   while (z == 0)
    //       PRT-LIT-NEXT:     ;
    //       PRT-LIT-NEXT:   while (y == 0)
    //       PRT-LIT-NEXT:     ;
    //       PRT-LIT-NEXT:   x = x * 10 + 1;
    //       PRT-LIT-NEXT: }

    // PRT-NOACC-VAR-NEXT: {
    // PRT-NOACC-VAR-NEXT:   while (z == 0)
    // PRT-NOACC-VAR-NEXT:     ;
    // PRT-NOACC-VAR-NEXT:   while (y == 0)
    // PRT-NOACC-VAR-NEXT:     ;
    // PRT-NOACC-VAR-NEXT:   x = x * 10 + 1;
    // PRT-NOACC-VAR-NEXT: }

    //    PRT-AO-VAR-NEXT: // v----------ACC----------v
    //     PRT-A-VAR-NEXT: #pragma acc parallel num_gangs(1) present(x,y,z) async({{Q0|q0}})
    //     PRT-A-VAR-NEXT: {
    //     PRT-A-VAR-NEXT:   while (z == 0)
    //     PRT-A-VAR-NEXT:     ;
    //     PRT-A-VAR-NEXT:   while (y == 0)
    //     PRT-A-VAR-NEXT:     ;
    //     PRT-A-VAR-NEXT:   x = x * 10 + 1;
    //     PRT-A-VAR-NEXT: }
    //    PRT-AO-VAR-NEXT: // ---------ACC->OMP--------
    //    PRT-AO-VAR-NEXT: // {
    //    PRT-AO-VAR-NEXT: //   #pragma omp target teams num_teams(1) map(present,ompx_hold,alloc: x,y,z) nowait depend(inout : *acc2omp_async2dep(q0))
    //    PRT-AO-VAR-NEXT: //   {
    //    PRT-AO-VAR-NEXT: //     while (z == 0)
    //    PRT-AO-VAR-NEXT: //       ;
    //    PRT-AO-VAR-NEXT: //     while (y == 0)
    //    PRT-AO-VAR-NEXT: //       ;
    //    PRT-AO-VAR-NEXT: //     x = x * 10 + 1;
    //    PRT-AO-VAR-NEXT: //   }
    //    PRT-AO-VAR-NEXT: //   #pragma omp taskwait depend(inout : *acc2omp_async2dep([[ACC_ASYNC_SYNC]]))
    //    PRT-AO-VAR-NEXT: // }
    //    PRT-AO-VAR-NEXT: // ^----------OMP----------^

    //    PRT-OA-VAR-NEXT: // v----------OMP----------v
    //     PRT-O-VAR-NEXT: {
    //     PRT-O-VAR-NEXT:   #pragma omp target teams num_teams(1) map(present,ompx_hold,alloc: x,y,z) nowait depend(inout : *acc2omp_async2dep(q0))
    //     PRT-O-VAR-NEXT:   {
    //     PRT-O-VAR-NEXT:     while (z == 0)
    //     PRT-O-VAR-NEXT:       ;
    //     PRT-O-VAR-NEXT:     while (y == 0)
    //     PRT-O-VAR-NEXT:       ;
    //     PRT-O-VAR-NEXT:     x = x * 10 + 1;
    //     PRT-O-VAR-NEXT:   }
    //     PRT-O-VAR-NEXT:   #pragma omp taskwait depend(inout : *acc2omp_async2dep([[ACC_ASYNC_SYNC]]))
    //     PRT-O-VAR-NEXT: }
    //    PRT-OA-VAR-NEXT: // ---------OMP<-ACC--------
    //    PRT-OA-VAR-NEXT: // #pragma acc parallel num_gangs(1) present(x,y,z) async({{Q0|q0}})
    //    PRT-OA-VAR-NEXT: // {
    //    PRT-OA-VAR-NEXT: //   while (z == 0)
    //    PRT-OA-VAR-NEXT: //     ;
    //    PRT-OA-VAR-NEXT: //   while (y == 0)
    //    PRT-OA-VAR-NEXT: //     ;
    //    PRT-OA-VAR-NEXT: //   x = x * 10 + 1;
    //    PRT-OA-VAR-NEXT: // }
    //    PRT-OA-VAR-NEXT: // ^----------ACC----------^

    //        PRT-NEXT: fcBoundary();
    fcBoundary();
    #pragma acc parallel num_gangs(1) present(x,y,z) async(Q0)
    {
      while (z == 0) // make sure host isn't scheduled to run afterward
        ;
      while (y == 0) // make sure queue 1 isn't scheduled to run afterward
        ;
      x = x * 10 + 1;
    }
    fcBoundary();

    #pragma acc parallel num_gangs(1) present(x,y,z) async(Q1)
    {
      while (z == 0) // make sure host isn't scheduled to run afterward
        ;
      y = y * 10 + 1;
      while (x == 0) // make sure queue 0 isn't scheduled to run afterward
        ;
    }
    #pragma acc parallel num_gangs(1) present(x) async(Q0)
    x = x * 10 + 2;
    #pragma acc parallel num_gangs(1) present(y) async(Q1)
    y = y * 10 + 2;

    z = 1;
    #pragma acc update device(z)
    while (y == 0) { // make sure queue 1 starts running before acc wait
      #pragma acc update host(y)
    }
    while (x == 0) { // make sure queue 0 starts running before acc wait
      #pragma acc update host(x)
    }

    //      DMP: ACCWaitDirective
    // DMP-NEXT:   impl: OMPTaskwaitDirective
    // DMP-NEXT: CallExpr
    //
    //         PRT: fcBoundary();
    //  PRT-A-NEXT: #pragma acc wait
    // PRT-AO-NEXT: // #pragma omp taskwait
    //  PRT-O-NEXT: #pragma omp taskwait
    // PRT-OA-NEXT: // #pragma acc wait
    //    PRT-NEXT: fcBoundary();
    fcBoundary();
    #pragma acc wait
    fcBoundary();

    // EXE-NEXT:two queues and host: x=12, y=12, z=1
    #pragma acc exit data copyout(x,y)
    printf("two queues and host: x=%d, y=%d, z=%d\n", x, y, z);
  }
  return 0;
}
