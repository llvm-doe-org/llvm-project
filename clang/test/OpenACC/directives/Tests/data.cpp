// Check "acc data" cases not covered by data.c because they are specific to
// C++.
//
// In some cases, it's challenging to check when memory is mapped (e.g., for
// no_create), but we want to be sure the OpenMP implementation is doing what we
// expect (e.g., for lambdas).  Specifically, calling acc_is_present or
// omp_target_is_present or specifying a present clause on a directive only
// works from the host, so it doesn't help for checking no_create on "acc
// parallel".  We could examine the output of LIBOMPTARGET_DEBUG=1, but it only
// works in debug builds.  Our solution is to utilize the OpenACC Profiling
// Interface, which we usually only exercise in the runtime test suite.
//
// REDEFINE: %{dmp:fc:args} = -strict-whitespace
// REDEFINE: %{exe:fc:args} = -strict-whitespace -match-full-lines \
// REDEFINE:                  -implicit-check-not='{{.}}'
// RUN: %{acc-check-dmp-cxx}
// RUN: %{acc-check-prt-cxx}
// RUN: %{acc-check-exe-cxx}
//
// END.

/* expected-no-diagnostics */

#include <acc_prof.h>
#include <stdio.h>
#include <string.h>

#define DEF_CALLBACK(Event)                                   \
static void on_##Event(acc_prof_info *pi, acc_event_info *ei, \
                       acc_api_info *ai) {                    \
  printf("acc_ev_" #Event "\n");                     \
}

#define REG_CALLBACK(Event) reg(acc_ev_##Event, on_##Event, acc_reg)

DEF_CALLBACK(create)
DEF_CALLBACK(delete)
DEF_CALLBACK(enter_data_start)
DEF_CALLBACK(exit_data_start)

void acc_register_library(acc_prof_reg reg, acc_prof_reg unreg,
                          acc_prof_lookup lookup) {
  REG_CALLBACK(create);
  REG_CALLBACK(delete);
  REG_CALLBACK(enter_data_start);
  REG_CALLBACK(exit_data_start);
}

//------------------------------------------------------------------------------
// Check that an OpenACC directive within a lambda within an OpenACC construct
// (acc data) translates successfully.
//
// Specifically, check that it doesn't try to translate the inner directive
// twice, once at the inner directive because it's the outermost in the lambda,
// and then again as part of the outer construct translation.  If it does try it
// twice, hopefully it fails an assertion where it tries to attach a second
// OpenMP node to the OpenACC node.
//------------------------------------------------------------------------------

// DMP-LABEL: FunctionDecl {{.*}} directiveInLambdaInConstruct 'void ()'
//  DMP-NEXT:   CompoundStmt
//       DMP:     ACCDataDirective
//  DMP-NEXT:       ACCCopyClause
//  DMP-NEXT:         DeclRefExpr {{.*}} 'x'
//  DMP-NEXT:       impl: OMPTargetDataDirective
//  DMP-NEXT:         OMPMapClause
//  DMP-NEXT:           DeclRefExpr {{.*}} 'x'
//       DMP:         CompoundStmt
//       DMP:           LambdaExpr
//  DMP-NEXT:             CXXRecordDecl
//       DMP:               CXXMethodDecl
//  DMP-NEXT:                 CompoundStmt
//  DMP-NEXT:                   OMPTargetTeamsDirective
//  DMP-NEXT:                     OMPNum_teamsClause
//  DMP-NEXT:                       IntegerLiteral {{.*}} 1
//  DMP-NEXT:                     OMPMapClause
//  DMP-NEXT:                       DeclRefExpr {{.*}} 'x'
//   DMP-NOT:                     OMP
//       DMP:                     CompoundAssignOperator {{.*}} '+='
//       DMP:       CompoundStmt
//       DMP:         LambdaExpr
//  DMP-NEXT:           CXXRecordDecl
//       DMP:             CXXMethodDecl
//  DMP-NEXT:               CompoundStmt
//       DMP:                 ACCParallelDirective
//  DMP-NEXT:                   ACCNumGangsClause
//  DMP-NEXT:                     IntegerLiteral {{.*}} 1
//  DMP-NEXT:                   ACCPresentClause
//  DMP-NEXT:                     DeclRefExpr {{.*}} 'x'
//  DMP-NEXT:                   ACCSharedClause {{.*}} <implicit>
//  DMP-NEXT:                     DeclRefExpr {{.*}} 'x'
//  DMP-NEXT:                   impl: OMPTargetTeamsDirective
//  DMP-NEXT:                     OMPNum_teamsClause
//  DMP-NEXT:                       IntegerLiteral {{.*}} 1
//  DMP-NEXT:                     OMPMapClause
//  DMP-NEXT:                       DeclRefExpr {{.*}} 'x'
//   DMP-NOT:                     OMP
//       DMP:                     CompoundAssignOperator {{.*}} '+='
//
//   PRT-LABEL: void directiveInLambdaInConstruct() {
//    PRT-NEXT:   printf
//    PRT-NEXT:   int x =
//  PRT-A-NEXT:   {{^ *}}#pragma acc data copy(x){{$}}
// PRT-AO-NEXT:   {{^ *}}// #pragma omp target data map(ompx_hold,tofrom: x){{$}}
//  PRT-O-NEXT:   {{^ *}}#pragma omp target data map(ompx_hold,tofrom: x){{$}}
// PRT-OA-NEXT:   {{^ *}}// #pragma acc data copy(x){{$}}
//    PRT-NEXT:   {
//    PRT-NEXT:     [&]() {
//  PRT-A-NEXT:       {{^ *}}#pragma acc parallel num_gangs(1) present(x){{$}}
// PRT-AO-NEXT:       {{^ *}}// #pragma omp target teams num_teams(1) map(present,ompx_hold,alloc: x){{$}}
//  PRT-O-NEXT:       {{^ *}}#pragma omp target teams num_teams(1) map(present,ompx_hold,alloc: x){{$}}
// PRT-OA-NEXT:       {{^ *}}// #pragma acc parallel num_gangs(1) present(x){{$}}
//    PRT-NEXT:       x +=
//    PRT-NEXT:     }();
//    PRT-NEXT:     printf
//    PRT-NEXT:   }
//    PRT-NEXT:   printf
//    PRT-NEXT: }
//
//     EXE-LABEL:directiveInLambdaInConstruct
//  EXE-OFF-NEXT:acc_ev_enter_data_start
//  EXE-OFF-NEXT:acc_ev_create
//  EXE-OFF-NEXT:acc_ev_enter_data_start
//  EXE-OFF-NEXT:acc_ev_exit_data_start
// EXE-HOST-NEXT:after lambda: x=104
//  EXE-OFF-NEXT:after lambda: x=99
//  EXE-OFF-NEXT:acc_ev_exit_data_start
//  EXE-OFF-NEXT:acc_ev_delete
//      EXE-NEXT:after acc data: x=104
void directiveInLambdaInConstruct() {
  printf("directiveInLambdaInConstruct\n");
  int x = 99;
  #pragma acc data copy(x)
  {
    [&]() {
      #pragma acc parallel num_gangs(1) present(x)
      x += 5;
    }();
    printf("after lambda: x=%d\n", x);
  }
  printf("after acc data: x=%d\n", x);
}

//------------------------------------------------------------------------------
// Check that a DMA from an OpenACC data construct enclosing a lambda is not
// visible on an OpenACC parallel construct within the lambda and does not
// suppress implicit DMAs there.
//
// Skip dump checks.  The previous case is similar enough that print checks
// should suffice to check for a correct translation this time.
//------------------------------------------------------------------------------

// DMP-LABEL: FunctionDecl {{.*}} dmaNotVisibleInLambda
//
//   PRT-LABEL: void dmaNotVisibleInLambda() {
//    PRT-NEXT:   printf
//    PRT-NEXT:   int x[] = {{{.*}}};
//    PRT-NEXT:   int y[] = {{{.*}}};
//  PRT-A-NEXT:   {{^ *}}#pragma acc data no_create(x,y){{$}}
// PRT-AO-NEXT:   {{^ *}}// #pragma omp target data map(ompx_no_alloc,ompx_hold,alloc: x,y){{$}}
//  PRT-O-NEXT:   {{^ *}}#pragma omp target data map(ompx_no_alloc,ompx_hold,alloc: x,y){{$}}
// PRT-OA-NEXT:   {{^ *}}// #pragma acc data no_create(x,y){{$}}
//    PRT-NEXT:   {
//    PRT-NEXT:     [&x, y]() mutable {
//  PRT-A-NEXT:       {{^ *}}#pragma acc parallel num_gangs(1){{$}}
// PRT-AO-NEXT:       {{^ *}}// #pragma omp target teams num_teams(1) map(ompx_hold,tofrom: x,y){{$}}
//  PRT-O-NEXT:       {{^ *}}#pragma omp target teams num_teams(1) map(ompx_hold,tofrom: x,y){{$}}
// PRT-OA-NEXT:       {{^ *}}// #pragma acc parallel num_gangs(1){{$}}
//    PRT-NEXT:       {
//    PRT-NEXT:         x[0] +=
//    PRT-NEXT:         y[0] +=
//    PRT-NEXT:       }
//    PRT-NEXT:     }();
//    PRT-NEXT:   }
//    PRT-NEXT:   printf
//    PRT-NEXT:   printf
//    PRT-NEXT: }
//
//    EXE-LABEL:dmaNotVisibleInLambda
// EXE-OFF-NEXT:acc_ev_enter_data_start
// EXE-OFF-NEXT:acc_ev_enter_data_start
// EXE-OFF-NEXT:acc_ev_create
// EXE-OFF-NEXT:acc_ev_create
// EXE-OFF-NEXT:acc_ev_exit_data_start
// EXE-OFF-NEXT:acc_ev_delete
// EXE-OFF-NEXT:acc_ev_delete
// EXE-OFF-NEXT:acc_ev_exit_data_start
//     EXE-NEXT:x[0]=104
//     EXE-NEXT:y[0]=88
void dmaNotVisibleInLambda() {
  printf("dmaNotVisibleInLambda\n");
  int x[] = {99};
  int y[] = {88};
  #pragma acc data no_create(x,y)
  {
    [&x, y]() mutable {
      #pragma acc parallel num_gangs(1)
      {
        x[0] += 5;
        y[0] += 6;
      }
    }();
  }
  printf("x[0]=%d\n", x[0]);
  printf("y[0]=%d\n", y[0]);
}

//------------------------------------------------------------------------------
// Check that a lambda can be defined and assigned to a variable within a data
// construct.
//
// The transformation to OpenMP used to produce a spurious type conversion error
// diagnostic because the variable's type wasn't transformed to the transformed
// lambda's type.
//------------------------------------------------------------------------------

// DMP-LABEL: FunctionDecl {{.*}} lambdaAssignInAccData
//
//   PRT-LABEL: void lambdaAssignInAccData() {
//    PRT-NEXT:   printf
//    PRT-NEXT:   int x;
//  PRT-A-NEXT:   {{^ *}}#pragma acc data no_create(x){{$}}
// PRT-AO-NEXT:   {{^ *}}// #pragma omp target data map(ompx_no_alloc,ompx_hold,alloc: x){{$}}
//  PRT-O-NEXT:   {{^ *}}#pragma omp target data map(ompx_no_alloc,ompx_hold,alloc: x){{$}}
// PRT-OA-NEXT:   {{^ *}}// #pragma acc data no_create(x){{$}}
//    PRT-NEXT:   {
//    PRT-NEXT:     auto lambda = []() {
//    PRT-NEXT:       printf
//    PRT-NEXT:     };
//    PRT-NEXT:     lambda();
//    PRT-NEXT:   }
//    PRT-NEXT: }
//
//    EXE-LABEL:lambdaAssignInAccData
// EXE-OFF-NEXT:acc_ev_enter_data_start
//     EXE-NEXT:hello world
// EXE-OFF-NEXT:acc_ev_exit_data_start
void lambdaAssignInAccData() {
  printf("lambdaAssignInAccData\n");
  int x;
  #pragma acc data no_create(x)
  {
    auto lambda = []() {
      printf("hello world\n");
    };
    lambda();
  }
}

int main() {
  directiveInLambdaInConstruct();
  dmaNotVisibleInLambda();
  lambdaAssignInAccData();
  return 0;
}
