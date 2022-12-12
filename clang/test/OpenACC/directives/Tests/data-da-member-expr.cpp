// Check data clauses for member expressions and for 'this' on "acc data", but
// only check cases that are not covered by data-da-member-expr.c because they
// are specific to C++.

// REDEFINE: %{all:clang:args} = -gline-tables-only
// REDEFINE: %{dmp:fc:args} = -strict-whitespace
// REDEFINE: %{exe:fc:args} = -match-full-lines -implicit-check-not='{{.}}'
// RUN: %{acc-check-dmp-cxx}
// RUN: %{acc-check-prt-cxx}
// RUN: %{acc-check-exe-cxx}

// END.

/* expected-no-diagnostics */

#include <acc_prof.h>
#include <stdio.h>
#include <string.h>

static void print_event(const char *eventName, acc_event_info *ei) {
  printf("%s", eventName);
  switch (ei->event_type) {
  case acc_ev_create:
  case acc_ev_delete:
  case acc_ev_alloc:
  case acc_ev_free:
  case acc_ev_enqueue_upload_start:
  case acc_ev_enqueue_upload_end:
  case acc_ev_enqueue_download_start:
  case acc_ev_enqueue_download_end:
    printf(": var_name=%s, bytes=%zu",
           ei->data_event.var_name ? ei->data_event.var_name : "<null>",
           ei->data_event.bytes);
    break;
  default:
    break;
  }
  printf("\n");
}

#define DEF_CALLBACK(Event)                                               \
static void on_##Event(acc_prof_info *pi, acc_event_info *ei,             \
                       acc_api_info *ai) {                                \
  print_event("acc_ev_" #Event, ei);                                      \
}

#define REG_CALLBACK(Event) reg(acc_ev_##Event, on_##Event, acc_reg)

DEF_CALLBACK(create)
DEF_CALLBACK(delete)
DEF_CALLBACK(alloc)
DEF_CALLBACK(free)
DEF_CALLBACK(enter_data_start)
DEF_CALLBACK(enter_data_end)
DEF_CALLBACK(exit_data_start)
DEF_CALLBACK(exit_data_end)
DEF_CALLBACK(enqueue_upload_start)
DEF_CALLBACK(enqueue_upload_end)
DEF_CALLBACK(enqueue_download_start)
DEF_CALLBACK(enqueue_download_end)

void acc_register_library(acc_prof_reg reg, acc_prof_reg unreg,
                          acc_prof_lookup lookup) {
  REG_CALLBACK(create);
  REG_CALLBACK(delete);
  REG_CALLBACK(alloc);
  REG_CALLBACK(free);
  REG_CALLBACK(enter_data_start);
  REG_CALLBACK(enter_data_end);
  REG_CALLBACK(exit_data_start);
  REG_CALLBACK(exit_data_end);
  REG_CALLBACK(enqueue_upload_start);
  REG_CALLBACK(enqueue_upload_end);
  REG_CALLBACK(enqueue_download_start);
  REG_CALLBACK(enqueue_download_end);
}

class Test {
  // The presence of the 'before' and 'after' members checks that we don't get,
  // for example, data extension errors at run time.
  int before;
  int c;
  int nc;
  int after;
public:
  void run() {
    // EXE: sizeof(Test) = [[#%d,SIZEOF_TEST:]]
    // EXE: sizeof(c) = [[#%d,SIZEOF_C:]]
    // EXE: sizeof(nc) = [[#%d,SIZEOF_NC:]]
    printf("sizeof(Test) = %zu\n", sizeof(Test));
    printf("sizeof(c) = %zu\n", sizeof c);
    printf("sizeof(nc) = %zu\n", sizeof nc);

    //--------------------------------------------------------------------------
    // Check that data clauses for 'this[0:1]' suppress implicit data clauses
    // for 'this[0:1]' on nested acc parallel (and on its OpenMP translation).
    //--------------------------------------------------------------------------

    // DMP-LABEL: "implicit DA suppression: copy for this\n"
    // PRT-LABEL: "implicit DA suppression: copy for this\n"
    // EXE-LABEL: implicit DA suppression: copy for this
    printf("implicit DA suppression: copy for this\n");

    // PRT-NEXT: c =
    // PRT-NEXT: nc =
    c = 1;
    nc = 2;

    //      DMP: ACCDataDirective
    // DMP-NEXT:   ACCCopyClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:     OMPArraySectionExpr
    // DMP-NEXT:       CXXThisExpr {{.*}} 'Test *' this
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 0
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:       <<<NULL>>>
    // DMP-NEXT:   impl: OMPTargetDataDirective
    // DMP-NEXT:     OMPMapClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       OMPArraySectionExpr
    // DMP-NEXT:         CXXThisExpr {{.*}} 'Test *' this
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 0
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:         <<<NULL>>>
    //  DMP-NOT:     OMP
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc data copy(this[0:1]){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target data map(ompx_hold,tofrom: this[0:1]){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp target data map(ompx_hold,tofrom: this[0:1]){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc data copy(this[0:1]){{$}}
    //
    // EXE-OFF-NEXT: acc_ev_enter_data_start
    // EXE-OFF-NEXT:   acc_ev_alloc: var_name=this[0:1], bytes=[[#SIZEOF_TEST]]
    // EXE-OFF-NEXT:   acc_ev_create: var_name=this[0:1], bytes=[[#SIZEOF_TEST]]
    // EXE-OFF-NEXT:   acc_ev_enqueue_upload_start: var_name=this[0:1], bytes=[[#SIZEOF_TEST]]
    // EXE-OFF-NEXT:   acc_ev_enqueue_upload_end: var_name=this[0:1], bytes=[[#SIZEOF_TEST]]
    // EXE-OFF-NEXT: acc_ev_enter_data_end
    #pragma acc data copy(this[0:1])
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      // PRT-NEXT: c +=
      // PRT: nc +=
       c += 10;
      nc += 10;

      // Check suppression of implicit data clauses.
      //
      //      DMP: ACCParallelDirective
      // DMP-NEXT:   ACCNumGangsClause
      // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
      // DMP-NEXT:   ACCNomapClause {{.*}} <implicit>
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
      // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
      // DMP-NEXT:     OMPMapClause
      //  DMP-NOT:       <implicit>
      // DMP-NEXT:       OMPArraySectionExpr
      // DMP-NEXT:         CXXThisExpr {{.*}} 'Test *' this
      // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 0
      // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 1
      // DMP-NEXT:         <<<NULL>>>
      //  DMP-NOT:     OMP
      //      DMP:     CompoundAssignOperator
      //
      //  PRT-A-NEXT: {{^ *}}#pragma acc parallel num_gangs(1){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(1) map(alloc: this[0:1]){{$}}
      //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(1) map(alloc: this[0:1]){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(1){{$}}
      //    PRT-NEXT: {
      //         PRT: }
      //
      // EXE-OFF-NEXT: acc_ev_enter_data_start
      // EXE-OFF-NEXT: acc_ev_enter_data_end
      // EXE-OFF-NEXT: acc_ev_exit_data_start
      // EXE-OFF-NEXT: acc_ev_exit_data_end
      #pragma acc parallel num_gangs(1)
      {
        class Test that = *this;
        this->c = that.c + 100;
        nc = that.nc + 100;
      }

      // PRT-NEXT: printf(
      //      PRT: );
      // PRT-NEXT: c +=
      // PRT-NEXT: nc +=
      //
      //      EXE-NEXT: After first acc parallel:
      // EXE-HOST-NEXT:   c=111, nc=112
      //  EXE-OFF-NEXT:   c= 11, nc= 12
      printf("After first acc parallel:\n"
             "  c=%3d, nc=%3d\n",
             c, nc);
      c += 10;
      nc += 10;

      // Check suppression for explicit data clauses: only data transfers are
      // suppressed.
      //
      //      DMP: ACCParallelDirective
      // DMP-NEXT:   ACCNumGangsClause
      // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
      // DMP-NEXT:   ACCCopyClause
      //  DMP-NOT:     <implicit>
      // DMP-NEXT:     OMPArraySectionExpr
      // DMP-NEXT:       CXXThisExpr {{.*}} 'Test *' this
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
      // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
      // DMP-NEXT:     OMPMapClause
      //  DMP-NOT:       <implicit>
      // DMP-NEXT:       OMPArraySectionExpr
      // DMP-NEXT:         CXXThisExpr {{.*}} 'Test *' this
      // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 0
      // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 1
      // DMP-NEXT:         <<<NULL>>>
      //  DMP-NOT:     OMP
      //      DMP:     CompoundAssignOperator
      //
      //  PRT-A-NEXT: {{^ *}}#pragma acc parallel num_gangs(1) copy(this[0:1]){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(1) map(ompx_hold,tofrom: this[0:1]){{$}}
      //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(1) map(ompx_hold,tofrom: this[0:1]){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(1) copy(this[0:1]){{$}}
      //    PRT-NEXT: {
      //         PRT: }
      //
      // EXE-OFF-NEXT: acc_ev_enter_data_start
      // EXE-OFF-NEXT: acc_ev_enter_data_end
      // EXE-OFF-NEXT: acc_ev_exit_data_start
      // EXE-OFF-NEXT: acc_ev_exit_data_end
      #pragma acc parallel num_gangs(1) copy(this[0:1])
      {
        c += 100;
        nc += 100;
      }

      // PRT-NEXT: printf(
      // PRT:      );
      // PRT-NEXT: c +=
      // PRT-NEXT: nc +=
      //
      //      EXE-NEXT: After second acc parallel:
      // EXE-HOST-NEXT:   c=221, nc=222
      //  EXE-OFF-NEXT:   c= 21, nc= 22
      printf("After second acc parallel:\n"
             "  c=%3d, nc=%3d\n",
             c, nc);
      c += 10;
      nc += 10;
    } // PRT-NEXT: }
    // EXE-OFF-NEXT: acc_ev_exit_data_start
    // EXE-OFF-NEXT:   acc_ev_enqueue_download_start: var_name=this[0:1], bytes=[[#SIZEOF_TEST]]
    // EXE-OFF-NEXT:   acc_ev_enqueue_download_end: var_name=this[0:1], bytes=[[#SIZEOF_TEST]]
    // EXE-OFF-NEXT:   acc_ev_delete: var_name=this[0:1], bytes=[[#SIZEOF_TEST]]
    // EXE-OFF-NEXT:   acc_ev_free: var_name=this[0:1], bytes=[[#SIZEOF_TEST]]
    // EXE-OFF-NEXT: acc_ev_exit_data_end

    // PRT-NEXT: printf(
    // PRT:      );
    //
    //      EXE-NEXT: After acc data:
    // EXE-HOST-NEXT:   c=231, nc=232
    //  EXE-OFF-NEXT:   c=201, nc=202
    printf("After acc data:\n"
           "  c=%3d, nc=%3d\n",
           c, nc);

    // DMP-LABEL: "implicit DA suppression: no_create for this\n"
    // PRT-LABEL: "implicit DA suppression: no_create for this\n"
    // EXE-LABEL: implicit DA suppression: no_create for this
    printf("implicit DA suppression: no_create for this\n");

    // PRT-NEXT: c =
    // PRT-NEXT: nc =
    c = 1;
    nc = 2;

    //      DMP: ACCDataDirective
    // DMP-NEXT:   ACCNoCreateClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:     OMPArraySectionExpr
    // DMP-NEXT:       CXXThisExpr {{.*}} 'Test *' this
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 0
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:       <<<NULL>>>
    // DMP-NEXT:   impl: OMPTargetDataDirective
    // DMP-NEXT:     OMPMapClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       OMPArraySectionExpr
    // DMP-NEXT:         CXXThisExpr {{.*}} 'Test *' this
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 0
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:         <<<NULL>>>
    //  DMP-NOT:     OMP
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc data no_create(this[0:1]){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target data map(ompx_no_alloc,ompx_hold,alloc: this[0:1]){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp target data map(ompx_no_alloc,ompx_hold,alloc: this[0:1]){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc data no_create(this[0:1]){{$}}
    //
    // EXE-OFF-NEXT: acc_ev_enter_data_start
    // EXE-OFF-NEXT: acc_ev_enter_data_end
    #pragma acc data no_create(this[0:1])
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      // PRT-NEXT: c +=
      // PRT: nc +=
       c += 10;
      nc += 10;

      // Check suppression of implicit data clauses.
      //
      //      DMP: ACCParallelDirective
      // DMP-NEXT:   ACCNumGangsClause
      // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
      // DMP-NEXT:   ACCNomapClause {{.*}} <implicit>
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
      // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
      // DMP-NEXT:     OMPMapClause
      //  DMP-NOT:       <implicit>
      // DMP-NEXT:       OMPArraySectionExpr
      // DMP-NEXT:         CXXThisExpr {{.*}} 'Test *' this
      // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 0
      // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 1
      // DMP-NEXT:         <<<NULL>>>
      //  DMP-NOT:     OMP
      //      DMP:     CompoundAssignOperator
      //
      //  PRT-A-NEXT: {{^ *}}#pragma acc parallel num_gangs(1){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(1) map(ompx_no_alloc,alloc: this[0:1]){{$}}
      //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(1) map(ompx_no_alloc,alloc: this[0:1]){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(1){{$}}
      //    PRT-NEXT: {
      //         PRT:   if (0) {
      //         PRT:   }
      //    PRT-NEXT: }
      //
      // EXE-OFF-NEXT: acc_ev_enter_data_start
      // EXE-OFF-NEXT: acc_ev_enter_data_end
      // EXE-OFF-NEXT: acc_ev_exit_data_start
      // EXE-OFF-NEXT: acc_ev_exit_data_end
      #pragma acc parallel num_gangs(1)
      {
        // Reference this to trigger any implicit attributes, but this[0:1]
        // shouldn't be allocated, so don't actually access it at run time.
        if (0) {
          class Test that = *this;
          this->c = that.c + 100;
          nc = that.nc + 100;
        }
      }

      // PRT-NEXT: printf(
      //      PRT: );
      // PRT-NEXT: c +=
      // PRT-NEXT: nc +=
      //
      // EXE-NEXT: After first acc parallel:
      // EXE-NEXT:   c= 11, nc= 12
      printf("After first acc parallel:\n"
             "  c=%3d, nc=%3d\n",
             c, nc);
      c += 10;
      nc += 10;

      // Check suppression for explicit data clauses: no_create suppresses
      // nothing.
      //
      //      DMP: ACCParallelDirective
      // DMP-NEXT:   ACCNumGangsClause
      // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
      // DMP-NEXT:   ACCCopyClause
      //  DMP-NOT:     <implicit>
      // DMP-NEXT:     OMPArraySectionExpr
      // DMP-NEXT:       CXXThisExpr {{.*}} 'Test *' this
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
      // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
      // DMP-NEXT:     OMPMapClause
      //  DMP-NOT:       <implicit>
      // DMP-NEXT:       OMPArraySectionExpr
      // DMP-NEXT:         CXXThisExpr {{.*}} 'Test *' this
      // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 0
      // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 1
      // DMP-NEXT:         <<<NULL>>>
      //  DMP-NOT:     OMP
      //      DMP:     CompoundAssignOperator
      //
      //  PRT-A-NEXT: {{^ *}}#pragma acc parallel num_gangs(1) copy(this[0:1]){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(1) map(ompx_hold,tofrom: this[0:1]){{$}}
      //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(1) map(ompx_hold,tofrom: this[0:1]){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(1) copy(this[0:1]){{$}}
      //    PRT-NEXT: {
      //         PRT: }
      //
      // EXE-OFF-NEXT: acc_ev_enter_data_start
      // EXE-OFF-NEXT:   acc_ev_alloc: var_name=this[0:1], bytes=[[#SIZEOF_TEST]]
      // EXE-OFF-NEXT:   acc_ev_create: var_name=this[0:1], bytes=[[#SIZEOF_TEST]]
      // EXE-OFF-NEXT:   acc_ev_enqueue_upload_start: var_name=this[0:1], bytes=[[#SIZEOF_TEST]]
      // EXE-OFF-NEXT:   acc_ev_enqueue_upload_end: var_name=this[0:1], bytes=[[#SIZEOF_TEST]]
      // EXE-OFF-NEXT: acc_ev_enter_data_end
      // EXE-OFF-NEXT: acc_ev_exit_data_start
      // EXE-OFF-NEXT:   acc_ev_enqueue_download_start: var_name=this[0:1], bytes=[[#SIZEOF_TEST]]
      // EXE-OFF-NEXT:   acc_ev_enqueue_download_end: var_name=this[0:1], bytes=[[#SIZEOF_TEST]]
      // EXE-OFF-NEXT:   acc_ev_delete: var_name=this[0:1], bytes=[[#SIZEOF_TEST]]
      // EXE-OFF-NEXT:   acc_ev_free: var_name=this[0:1], bytes=[[#SIZEOF_TEST]]
      // EXE-OFF-NEXT: acc_ev_exit_data_end
      #pragma acc parallel num_gangs(1) copy(this[0:1])
      {
        c += 100;
        nc += 100;
      }

      // PRT-NEXT: printf(
      // PRT:      );
      // PRT-NEXT: c +=
      // PRT-NEXT: nc +=
      //
      // EXE-NEXT: After second acc parallel:
      // EXE-NEXT:   c=121, nc=122
      printf("After second acc parallel:\n"
             "  c=%3d, nc=%3d\n",
             c, nc);
      c += 10;
      nc += 10;
    } // PRT-NEXT: }
    // EXE-OFF-NEXT: acc_ev_exit_data_start
    // EXE-OFF-NEXT: acc_ev_exit_data_end

    // PRT-NEXT: printf(
    // PRT:      );
    //
    // EXE-NEXT: After acc data:
    // EXE-NEXT:   c=131, nc=132
    printf("After acc data:\n"
           "  c=%3d, nc=%3d\n",
           c, nc);

    //--------------------------------------------------------------------------
    // Check that data clauses with member expressions suppress implicit data
    // clauses for those member expressions on nested acc parallel (and on its
    // OpenMP translation).
    //--------------------------------------------------------------------------

    // DMP-LABEL: "implicit DA suppression: copy for member on implicit this\n"
    // PRT-LABEL: "implicit DA suppression: copy for member on implicit this\n"
    // EXE-LABEL: implicit DA suppression: copy for member on implicit this
    printf("implicit DA suppression: copy for member on implicit this\n");

    // PRT-NEXT: c =
    c = 10;

    //      DMP: ACCDataDirective
    // DMP-NEXT:   ACCCopyClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:     MemberExpr {{.* ->c }}
    // DMP-NEXT:       CXXThisExpr {{.*}} 'Test *' implicit this
    // DMP-NEXT:   impl: OMPTargetDataDirective
    // DMP-NEXT:     OMPMapClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       MemberExpr {{.* ->c }}
    // DMP-NEXT:         CXXThisExpr {{.*}} 'Test *' implicit this
    //  DMP-NOT:     OMP
    //
    // PRT-A-AST-NEXT: {{^ *}}#pragma acc data copy(this->c){{$}}
    // PRT-A-SRC-NEXT: {{^ *}}#pragma acc data copy(c){{$}}
    //    PRT-AO-NEXT: {{^ *}}// #pragma omp target data
    //    PRT-AO-SAME: {{^  *}}map(ompx_hold,tofrom: this->c){{$}}
    //
    //      PRT-O-NEXT: {{^ *}}#pragma omp target data
    //      PRT-O-SAME: {{^  *}}map(ompx_hold,tofrom: this->c){{$}}
    // PRT-OA-AST-NEXT: {{^ *}}// #pragma acc data copy(this->c){{$}}
    // PRT-OA-SRC-NEXT: {{^ *}}// #pragma acc data copy(c){{$}}
    //
    // EXE-OFF-NEXT: acc_ev_enter_data_start
    // EXE-OFF-NEXT:   acc_ev_alloc: var_name=this->c, bytes=[[#SIZEOF_C]]
    // EXE-OFF-NEXT:   acc_ev_create: var_name=this->c, bytes=[[#SIZEOF_C]]
    // EXE-OFF-NEXT:   acc_ev_enqueue_upload_start: var_name=this->c, bytes=[[#SIZEOF_C]]
    // EXE-OFF-NEXT:   acc_ev_enqueue_upload_end: var_name=this->c, bytes=[[#SIZEOF_C]]
    // EXE-OFF-NEXT: acc_ev_enter_data_end
    #pragma acc data copy(c)
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      // PRT-NEXT: c +=
       c += 100;

      // Check suppression of implicit data clauses.
      //
      //      DMP: ACCParallelDirective
      // DMP-NEXT:   ACCNumGangsClause
      // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
      // DMP-NEXT:   ACCNomapClause {{.*}} <implicit>
      // DMP-NEXT:     MemberExpr {{.* ->c }}
      // DMP-NEXT:       CXXThisExpr {{.*}} 'Test *' implicit this
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     MemberExpr {{.* ->c }}
      // DMP-NEXT:       CXXThisExpr {{.*}} 'Test *' implicit this
      // DMP-NEXT:   impl: OMPTargetTeamsDirective
      // DMP-NEXT:     OMPNum_teamsClause
      // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
      // DMP-NEXT:     OMPMapClause
      //  DMP-NOT:       <implicit>
      // DMP-NEXT:       MemberExpr {{.* ->c }}
      // DMP-NEXT:         CXXThisExpr {{.*}} 'Test *' implicit this
      //  DMP-NOT:     OMP
      //      DMP:     CompoundAssignOperator
      //
      //  PRT-A-NEXT: {{^ *}}#pragma acc parallel num_gangs(1){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(1) map(alloc: this->c){{$}}
      //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(1) map(alloc: this->c){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(1){{$}}
      //
      // PRT-NEXT: c +=
      //
      // EXE-OFF-NEXT: acc_ev_enter_data_start
      // EXE-OFF-NEXT: acc_ev_enter_data_end
      // EXE-OFF-NEXT: acc_ev_exit_data_start
      // EXE-OFF-NEXT: acc_ev_exit_data_end
      #pragma acc parallel num_gangs(1)
      c += 1000;

      // PRT-NEXT: printf(
      // PRT:      );
      // PRT-NEXT: c +=
      //
      //      EXE-NEXT: After first acc parallel:
      // EXE-HOST-NEXT:   c=1110
      //  EXE-OFF-NEXT:   c= 110
      printf("After first acc parallel:\n"
             "  c=%4d\n",
             c);
      c += 100;

      // Check suppression for explicit data clauses: only data transfers are
      // suppressed.
      //
      //      DMP: ACCParallelDirective
      // DMP-NEXT:   ACCNumGangsClause
      // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
      // DMP-NEXT:   ACCCopyClause
      //  DMP-NOT:     <implicit>
      // DMP-NEXT:     MemberExpr {{.* ->c }}
      // DMP-NEXT:       CXXThisExpr {{.*}} 'Test *' implicit this
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     MemberExpr {{.* ->c }}
      // DMP-NEXT:       CXXThisExpr {{.*}} 'Test *' implicit this
      // DMP-NEXT:   impl: OMPTargetTeamsDirective
      // DMP-NEXT:     OMPNum_teamsClause
      // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
      // DMP-NEXT:     OMPMapClause
      //  DMP-NOT:       <implicit>
      // DMP-NEXT:       MemberExpr {{.* ->c }}
      // DMP-NEXT:         CXXThisExpr {{.*}} 'Test *' implicit this
      //  DMP-NOT:     OMP
      //      DMP:     CompoundAssignOperator
      //
      // PRT-A-AST-NEXT: {{^ *}}#pragma acc parallel num_gangs(1) copy(this->c){{$}}
      // PRT-A-SRC-NEXT: {{^ *}}#pragma acc parallel num_gangs(1) copy(c){{$}}
      //    PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(1)
      //    PRT-AO-SAME: {{^ *}}map(ompx_hold,tofrom: this->c){{$}}
      //
      //      PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(1)
      //      PRT-O-SAME: {{^ *}}map(ompx_hold,tofrom: this->c){{$}}
      // PRT-OA-AST-NEXT: {{^ *}}// #pragma acc parallel num_gangs(1) copy(this->c){{$}}
      // PRT-OA-SRC-NEXT: {{^ *}}// #pragma acc parallel num_gangs(1) copy(c){{$}}
      //
      // PRT-NEXT: c +=
      //
      // EXE-OFF-NEXT: acc_ev_enter_data_start
      // EXE-OFF-NEXT: acc_ev_enter_data_end
      // EXE-OFF-NEXT: acc_ev_exit_data_start
      // EXE-OFF-NEXT: acc_ev_exit_data_end
      #pragma acc parallel num_gangs(1) copy(c)
      c += 1000;

      // PRT-NEXT: printf(
      // PRT:      );
      // PRT-NEXT: c +=
      //
      //      EXE-NEXT: After second acc parallel:
      // EXE-HOST-NEXT:   c=2210
      //  EXE-OFF-NEXT:   c= 210
      printf("After second acc parallel:\n"
             "   c=%4d\n",
             c);
      c += 100;
    } // PRT-NEXT: }
    // EXE-OFF-NEXT: acc_ev_exit_data_start
    // EXE-OFF-NEXT:   acc_ev_enqueue_download_start: var_name=this->c, bytes=[[#SIZEOF_C]]
    // EXE-OFF-NEXT:   acc_ev_enqueue_download_end: var_name=this->c, bytes=[[#SIZEOF_C]]
    // EXE-OFF-NEXT:   acc_ev_delete: var_name=this->c, bytes=[[#SIZEOF_C]]
    // EXE-OFF-NEXT:   acc_ev_free: var_name=this->c, bytes=[[#SIZEOF_C]]
    // EXE-OFF-NEXT: acc_ev_exit_data_end

    // PRT-NEXT: printf(
    // PRT:      );
    //
    //      EXE-NEXT: After acc data:
    // EXE-HOST-NEXT:   c=2310
    //  EXE-OFF-NEXT:   c=2010
    printf("After acc data:\n"
           "  c=%4d\n",
           c);

    // DMP-LABEL: "implicit DA suppression: no_create for member on implicit this\n"
    // PRT-LABEL: "implicit DA suppression: no_create for member on implicit this\n"
    // EXE-LABEL: implicit DA suppression: no_create for member on implicit this
    printf("implicit DA suppression: no_create for member on implicit this\n");

    // PRT-NEXT: nc =
    nc = 50;

    //      DMP: ACCDataDirective
    // DMP-NEXT:   ACCNoCreateClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:     MemberExpr {{.* ->nc }}
    // DMP-NEXT:       CXXThisExpr {{.*}} 'Test *' implicit this
    // DMP-NEXT:   impl: OMPTargetDataDirective
    // DMP-NEXT:     OMPMapClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       MemberExpr {{.* ->nc }}
    // DMP-NEXT:         CXXThisExpr {{.*}} 'Test *' implicit this
    //  DMP-NOT:     OMP
    //
    // PRT-A-AST-NEXT: {{^ *}}#pragma acc data no_create(this->nc){{$}}
    // PRT-A-SRC-NEXT: {{^ *}}#pragma acc data no_create(nc){{$}}
    //    PRT-AO-NEXT: {{^ *}}// #pragma omp target data
    //    PRT-AO-SAME: {{^  *}}map(ompx_no_alloc,ompx_hold,alloc: this->nc){{$}}
    //
    //      PRT-O-NEXT: {{^ *}}#pragma omp target data
    //      PRT-O-SAME: {{^  *}}map(ompx_no_alloc,ompx_hold,alloc: this->nc){{$}}
    // PRT-OA-AST-NEXT: {{^ *}}// #pragma acc data no_create(this->nc){{$}}
    // PRT-OA-SRC-NEXT: {{^ *}}// #pragma acc data no_create(nc){{$}}
    //
    // EXE-OFF-NEXT: acc_ev_enter_data_start
    // EXE-OFF-NEXT: acc_ev_enter_data_end
    #pragma acc data no_create(nc)
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      // PRT-NEXT: nc +=
      nc += 100;

      // Check suppression of implicit data clauses.
      //
      //      DMP: ACCParallelDirective
      // DMP-NEXT:   ACCNumGangsClause
      // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
      // DMP-NEXT:   ACCNomapClause {{.*}} <implicit>
      // DMP-NEXT:     MemberExpr {{.* ->nc }}
      // DMP-NEXT:       CXXThisExpr {{.*}} 'Test *' implicit this
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     MemberExpr {{.* ->nc }}
      // DMP-NEXT:       CXXThisExpr {{.*}} 'Test *' implicit this
      // DMP-NEXT:   impl: OMPTargetTeamsDirective
      // DMP-NEXT:     OMPNum_teamsClause
      // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
      // DMP-NEXT:     OMPMapClause
      //  DMP-NOT:       <implicit>
      // DMP-NEXT:       MemberExpr {{.* ->nc }}
      // DMP-NEXT:         CXXThisExpr {{.*}} 'Test *' implicit this
      //  DMP-NOT:     OMP
      //      DMP:     CompoundStmt
      //
      //  PRT-A-NEXT: {{^ *}}#pragma acc parallel num_gangs(1){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(1)
      // PRT-AO-SAME: {{^ *}}map(ompx_no_alloc,alloc: this->nc){{$}}
      //
      //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(1)
      //  PRT-O-SAME: {{^ *}}map(ompx_no_alloc,alloc: this->nc){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(1){{$}}
      //
      // PRT-NEXT: {
      //      PRT: }
      //
      // EXE-OFF-NEXT: acc_ev_enter_data_start
      // EXE-OFF-NEXT: acc_ev_enter_data_end
      // EXE-OFF-NEXT: acc_ev_exit_data_start
      // EXE-OFF-NEXT: acc_ev_exit_data_end
      #pragma acc parallel num_gangs(1)
      {
        // Reference nc to trigger any implicit attributes, but nc shouldn't
        // be allocated, so don't actually access it at run time.
        if (0)
          nc = 55;
      }

      // PRT-NEXT: printf(
      // PRT:      );
      // PRT-NEXT: nc +=
      //
      // EXE-NEXT: After first acc parallel:
      // EXE-NEXT:   nc= 150
      printf("After first acc parallel:\n"
             "  nc=%4d\n",
             nc);
      nc += 100;

      // Check suppression for explicit data clauses: no_create suppresses
      // nothing.
      //
      //      DMP: ACCParallelDirective
      // DMP-NEXT:   ACCNumGangsClause
      // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
      // DMP-NEXT:   ACCCopyClause
      //  DMP-NOT:     <implicit>
      // DMP-NEXT:     MemberExpr {{.* ->nc }}
      // DMP-NEXT:       CXXThisExpr {{.*}} 'Test *' implicit this
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     MemberExpr {{.* ->nc }}
      // DMP-NEXT:       CXXThisExpr {{.*}} 'Test *' implicit this
      // DMP-NEXT:   impl: OMPTargetTeamsDirective
      // DMP-NEXT:     OMPNum_teamsClause
      // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
      // DMP-NEXT:     OMPMapClause
      //  DMP-NOT:       <implicit>
      // DMP-NEXT:       MemberExpr {{.* ->nc }}
      // DMP-NEXT:         CXXThisExpr {{.*}} 'Test *' implicit this
      //  DMP-NOT:     OMP
      //      DMP:     CompoundAssignOperator
      //
      // PRT-A-AST-NEXT: {{^ *}}#pragma acc parallel num_gangs(1) copy(this->nc){{$}}
      // PRT-A-SRC-NEXT: {{^ *}}#pragma acc parallel num_gangs(1) copy(nc){{$}}
      //    PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(1)
      //    PRT-AO-SAME: {{^ *}}map(ompx_hold,tofrom: this->nc){{$}}
      //
      //      PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(1)
      //      PRT-O-SAME: {{^ *}}map(ompx_hold,tofrom: this->nc){{$}}
      // PRT-OA-AST-NEXT: {{^ *}}// #pragma acc parallel num_gangs(1) copy(this->nc){{$}}
      // PRT-OA-SRC-NEXT: {{^ *}}// #pragma acc parallel num_gangs(1) copy(nc){{$}}
      //
      // PRT-NEXT: nc +=
      //
      // EXE-OFF-NEXT: acc_ev_enter_data_start
      // EXE-OFF-NEXT:   acc_ev_alloc: var_name=this->nc, bytes=[[#SIZEOF_NC]]
      // EXE-OFF-NEXT:   acc_ev_create: var_name=this->nc, bytes=[[#SIZEOF_NC]]
      // EXE-OFF-NEXT:   acc_ev_enqueue_upload_start: var_name=this->nc, bytes=[[#SIZEOF_NC]]
      // EXE-OFF-NEXT:   acc_ev_enqueue_upload_end: var_name=this->nc, bytes=[[#SIZEOF_NC]]
      // EXE-OFF-NEXT: acc_ev_enter_data_end
      // EXE-OFF-NEXT: acc_ev_exit_data_start
      // EXE-OFF-NEXT:   acc_ev_enqueue_download_start: var_name=this->nc, bytes=[[#SIZEOF_NC]]
      // EXE-OFF-NEXT:   acc_ev_enqueue_download_end: var_name=this->nc, bytes=[[#SIZEOF_NC]]
      // EXE-OFF-NEXT:   acc_ev_delete: var_name=this->nc, bytes=[[#SIZEOF_NC]]
      // EXE-OFF-NEXT:   acc_ev_free: var_name=this->nc, bytes=[[#SIZEOF_NC]]
      // EXE-OFF-NEXT: acc_ev_exit_data_end
      #pragma acc parallel num_gangs(1) copy(nc)
      nc += 1000;

      // PRT-NEXT: printf(
      // PRT:      );
      // PRT-NEXT: nc +=
      //
      // EXE-NEXT: After second acc parallel:
      // EXE-NEXT:   nc=1250
      printf("After second acc parallel:\n"
             "  nc=%4d\n",
             nc);
      nc += 100;
    } // PRT-NEXT: }
    // EXE-OFF-NEXT: acc_ev_exit_data_start
    // EXE-OFF-NEXT: acc_ev_exit_data_end

    // PRT-NEXT: printf(
    // PRT:      );
    //
    // EXE-NEXT: After acc data:
    // EXE-NEXT:   nc=1350
    printf("After acc data:\n"
           "  nc=%4d\n",
           nc);

    //--------------------------------------------------------------------------
    // We checked member expressions on implicit 'this' above.  Check that
    // explicit 'this' basically works, but don't bother checking suppression
    // again.
    //--------------------------------------------------------------------------

    // EXE: sizeof(c) = [[#%d,SIZEOF_C:]]
    // EXE: sizeof(nc) = [[#%d,SIZEOF_NC:]]
    printf("sizeof(c) = %zu\n", sizeof c);
    printf("sizeof(nc) = %zu\n", sizeof nc);
    // DMP-LABEL: "copy for member on explicit this\n"
    // PRT-LABEL: ("copy for member on explicit this\n");
    // EXE-LABEL: copy for member on explicit this
    printf("copy for member on explicit this\n");
    //      DMP: ACCDataDirective
    // DMP-NEXT:   ACCCopyClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:     MemberExpr {{.* ->c }}
    // DMP-NEXT:       CXXThisExpr {{.*}} 'Test *' this
    // DMP-NEXT:   impl: OMPTargetDataDirective
    // DMP-NEXT:     OMPMapClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       MemberExpr {{.* ->c }}
    // DMP-NEXT:         CXXThisExpr {{.*}} 'Test *' this
    //  DMP-NOT:     OMP
    //      DMP:     NullStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc data copy(this->c){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target data
    // PRT-AO-SAME: {{^  *}}map(ompx_hold,tofrom: this->c){{$}}
    //
    //  PRT-O-NEXT: {{^ *}}#pragma omp target data
    //  PRT-O-SAME: {{^ *}}map(ompx_hold,tofrom: this->c){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc data copy(this->c){{$}}
    //
    // PRT-NEXT: ;
    //
    // EXE-OFF-NEXT: acc_ev_enter_data_start
    // EXE-OFF-NEXT:   acc_ev_alloc: var_name=this->c, bytes=[[#SIZEOF_C]]
    // EXE-OFF-NEXT:   acc_ev_create: var_name=this->c, bytes=[[#SIZEOF_C]]
    // EXE-OFF-NEXT:   acc_ev_enqueue_upload_start: var_name=this->c, bytes=[[#SIZEOF_C]]
    // EXE-OFF-NEXT:   acc_ev_enqueue_upload_end: var_name=this->c, bytes=[[#SIZEOF_C]]
    // EXE-OFF-NEXT: acc_ev_enter_data_end
    // EXE-OFF-NEXT: acc_ev_exit_data_start
    // EXE-OFF-NEXT:   acc_ev_enqueue_download_start: var_name=this->c, bytes=[[#SIZEOF_C]]
    // EXE-OFF-NEXT:   acc_ev_enqueue_download_end: var_name=this->c, bytes=[[#SIZEOF_C]]
    // EXE-OFF-NEXT:   acc_ev_delete: var_name=this->c, bytes=[[#SIZEOF_C]]
    // EXE-OFF-NEXT:   acc_ev_free: var_name=this->c, bytes=[[#SIZEOF_C]]
    // EXE-OFF-NEXT: acc_ev_exit_data_end
    #pragma acc data copy(this->c)
    ;

    // DMP-LABEL: "copy for member on explicit this\n"
    // PRT-LABEL: ("copy for member on explicit this\n");
    // EXE-LABEL: copy for member on explicit this
    printf("copy for member on explicit this\n");
    //      DMP: ACCDataDirective
    // DMP-NEXT:   ACCNoCreateClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:     MemberExpr {{.* ->c }}
    // DMP-NEXT:       CXXThisExpr {{.*}} 'Test *' this
    // DMP-NEXT:   impl: OMPTargetDataDirective
    // DMP-NEXT:     OMPMapClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       MemberExpr {{.* ->c }}
    // DMP-NEXT:         CXXThisExpr {{.*}} 'Test *' this
    //  DMP-NOT:     OMP
    //      DMP:     NullStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc data no_create(this->c){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target data
    // PRT-AO-SAME: {{^  *}}map(ompx_no_alloc,ompx_hold,alloc: this->c){{$}}
    //
    //  PRT-O-NEXT: {{^ *}}#pragma omp target data
    //  PRT-O-SAME: {{^  *}}map(ompx_no_alloc,ompx_hold,alloc: this->c){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc data no_create(this->c){{$}}
    //
    // PRT-NEXT: ;
    //
    // EXE-OFF-NEXT: acc_ev_enter_data_start
    // EXE-OFF-NEXT: acc_ev_enter_data_end
    // EXE-OFF-NEXT: acc_ev_exit_data_start
    // EXE-OFF-NEXT: acc_ev_exit_data_end
    #pragma acc data no_create(this->c)
    ;
  }
};

int main() {
  Test test;
  test.run();
  return 0;
}
