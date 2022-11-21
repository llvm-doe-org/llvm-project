// Check implicit and explicit data attributes for member expressions on
// "acc data".
//
// data.c checks other cases with every data clause.  Here, we just check 'copy'
// (whose behavior should be representative of most data clauses) and
// 'no_create' (whose OpenMP translation has special cases, in particular for
// any nested acc parallel).
//
// It's challenging to check that there are no redundant allocations or
// transfers (i.e., for multiple members or the full struct).  Specifically,
// calling acc_is_present or omp_target_is_present or specifying a present
// clause on a directive only works from the host, so it doesn't help for
// checking while in "acc parallel".  We could examine the output of
// LIBOMPTARGET_DEBUG=1, but it only works in debug builds.  Our solution is to
// utilize the OpenACC Profiling Interface, which we usually only exercise in
// the runtime test suite.

// REDEFINE: %{all:clang:args} = -gline-tables-only
// REDEFINE: %{exe:fc:args} = -match-full-lines -implicit-check-not='{{.}}'
// RUN: %{acc-check-dmp}
// RUN: %{acc-check-prt}
// RUN: %{acc-check-exe}

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

// We use one member throughout these tests.  The presence of the other members
// checks that we don't get, for example, data extension errors at run time.
struct T { int before; int i; int after; };

int main() {
  // EXE: sizeof(T::i) = [[#%d,SIZEOF_T_I:]]
  printf("sizeof(T::i) = %zu\n", sizeof ((struct T *)0)->i);

  //----------------------------------------------------------------------------
  // Check that data clauses with member expressions suppress implicit data
  // clauses for those member expressions on nested acc parallel (and on its
  // OpenMP translation).
  //----------------------------------------------------------------------------

  // DMP-LABEL: "suppression\n"
  // PRT-LABEL: "suppression\n"
  // EXE-LABEL: suppression
  printf("suppression\n");

  // PRT-NEXT: {
  {
    // PRT: nc = {999, 50};
    struct T  c = {999, 10};
    struct T nc = {999, 50};
    //      DMP: ACCDataDirective
    // DMP-NEXT:   ACCCopyClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:     MemberExpr {{.* .i }}
    // DMP-NEXT:       DeclRefExpr {{.*}} 'c' 'struct T'
    // DMP-NEXT:   ACCNoCreateClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:     MemberExpr {{.* .i }}
    // DMP-NEXT:       DeclRefExpr {{.*}} 'nc' 'struct T'
    // DMP-NEXT:   impl: OMPTargetDataDirective
    // DMP-NEXT:     OMPMapClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       MemberExpr {{.* .i }}
    // DMP-NEXT:         DeclRefExpr {{.*}} 'c' 'struct T'
    // DMP-NEXT:     OMPMapClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       MemberExpr {{.* .i }}
    // DMP-NEXT:         DeclRefExpr {{.*}} 'nc' 'struct T'
    //  DMP-NOT:     OMP
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc data copy(c.i) no_create(nc.i){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target data
    // PRT-AO-SAME: {{^  *}}map(ompx_hold,tofrom: c.i)
    // PRT-AO-SAME: {{^  *}}map(ompx_no_alloc,ompx_hold,alloc: nc.i){{$}}
    //
    //  PRT-O-NEXT: {{^ *}}#pragma omp target data
    //  PRT-O-SAME: {{^  *}}map(ompx_hold,tofrom: c.i)
    //  PRT-O-SAME: {{^  *}}map(ompx_no_alloc,ompx_hold,alloc: nc.i){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc data copy(c.i) no_create(nc.i){{$}}
    //
    // EXE-OFF-NEXT: acc_ev_enter_data_start
    // EXE-OFF-NEXT:   acc_ev_alloc: var_name=c.i, bytes=[[#SIZEOF_T_I]]
    // EXE-OFF-NEXT:   acc_ev_create: var_name=c.i, bytes=[[#SIZEOF_T_I]]
    // EXE-OFF-NEXT:   acc_ev_enqueue_upload_start: var_name=c.i, bytes=[[#SIZEOF_T_I]]
    // EXE-OFF-NEXT:   acc_ev_enqueue_upload_end: var_name=c.i, bytes=[[#SIZEOF_T_I]]
    // EXE-OFF-NEXT: acc_ev_enter_data_end
    #pragma acc data copy(c.i) no_create(nc.i)
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      // PRT-NEXT: c.i +=
      // PRT: nc.i +=
       c.i += 100;
      nc.i += 100;

      // Check suppression of implicit data clauses.
      //
      //      DMP: ACCParallelDirective
      // DMP-NEXT:   ACCNumGangsClause
      // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
      // DMP-NEXT:   ACCNomapClause {{.*}} <implicit>
      // DMP-NEXT:     MemberExpr {{.* .i }}
      // DMP-NEXT:       DeclRefExpr {{.*}} 'c' 'struct T'
      // DMP-NEXT:     MemberExpr {{.* .i }}
      // DMP-NEXT:       DeclRefExpr {{.*}} 'nc' 'struct T'
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     MemberExpr {{.* .i }}
      // DMP-NEXT:       DeclRefExpr {{.*}} 'c' 'struct T'
      // DMP-NEXT:     MemberExpr {{.* .i }}
      // DMP-NEXT:       DeclRefExpr {{.*}} 'nc' 'struct T'
      // DMP-NEXT:   impl: OMPTargetTeamsDirective
      // DMP-NEXT:     OMPNum_teamsClause
      // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
      // DMP-NEXT:     OMPMapClause
      //  DMP-NOT:       <implicit>
      // DMP-NEXT:       MemberExpr {{.* .i }}
      // DMP-NEXT:         DeclRefExpr {{.*}} 'nc' 'struct T'
      // DMP-NEXT:     OMPMapClause {{.*}} <implicit>
      // DMP-NEXT:       DeclRefExpr {{.*}} 'c' 'struct T'
      //  DMP-NOT:     OMP
      //      DMP:     CompoundStmt
      //
      //  PRT-A-NEXT: {{^ *}}#pragma acc parallel num_gangs(1){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(1)
      // PRT-AO-SAME: {{^ *}}map(ompx_no_alloc,alloc: nc.i){{$}}
      //
      //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(1)
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
        // Reference nc to trigger any implicit attributes, but nc shouldn't be
        // allocated, so don't actually access it at run time.
        if (!c.i)
          nc.i = 55;
        c.i += 1000;
      }

      // PRT-NEXT: printf(
      // PRT:      );
      // PRT-NEXT: c.i +=
      // PRT-NEXT: nc.i +=
      //
      //      EXE-NEXT: After first acc parallel:
      // EXE-HOST-NEXT:    c.i=1110, nc.i= 150
      //  EXE-OFF-NEXT:    c.i= 110, nc.i= 150
      printf("After first acc parallel:\n"
             "   c.i=%4d, nc.i=%4d\n",
             c.i, nc.i);
      c.i += 100;
      nc.i += 100;

      // Check suppression for explicit data clauses (only data transfers are
      // suppressed except no_create suppresses nothing).
      //
      //      DMP: ACCParallelDirective
      // DMP-NEXT:   ACCNumGangsClause
      // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
      // DMP-NEXT:   ACCCopyClause
      //  DMP-NOT:     <implicit>
      // DMP-NEXT:     MemberExpr {{.* .i }}
      // DMP-NEXT:       DeclRefExpr {{.*}} 'c' 'struct T'
      // DMP-NEXT:     MemberExpr {{.* .i }}
      // DMP-NEXT:       DeclRefExpr {{.*}} 'nc' 'struct T'
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     MemberExpr {{.* .i }}
      // DMP-NEXT:       DeclRefExpr {{.*}} 'c' 'struct T'
      // DMP-NEXT:     MemberExpr {{.* .i }}
      // DMP-NEXT:       DeclRefExpr {{.*}} 'nc' 'struct T'
      // DMP-NEXT:   impl: OMPTargetTeamsDirective
      // DMP-NEXT:     OMPNum_teamsClause
      // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
      // DMP-NEXT:     OMPMapClause
      //  DMP-NOT:       <implicit>
      // DMP-NEXT:       MemberExpr {{.* .i }}
      // DMP-NEXT:         DeclRefExpr {{.*}} 'c' 'struct T'
      // DMP-NEXT:       MemberExpr {{.* .i }}
      // DMP-NEXT:         DeclRefExpr {{.*}} 'nc' 'struct T'
      //  DMP-NOT:     OMP
      //      DMP:     CompoundStmt
      //
      //  PRT-A-NEXT: {{^ *}}#pragma acc parallel num_gangs(1) copy(c.i,nc.i){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(1)
      // PRT-AO-SAME: {{^ *}}map(ompx_hold,tofrom: c.i,nc.i){{$}}
      //
      //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(1)
      //  PRT-O-SAME: {{^ *}}map(ompx_hold,tofrom: c.i,nc.i){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(1) copy(c.i,nc.i){{$}}
      //
      // PRT-NEXT: {
      //      PRT: }
      //
      // EXE-OFF-NEXT: acc_ev_enter_data_start
      // EXE-OFF-NEXT:   acc_ev_alloc: var_name=nc.i, bytes=[[#SIZEOF_T_I]]
      // EXE-OFF-NEXT:   acc_ev_create: var_name=nc.i, bytes=[[#SIZEOF_T_I]]
      // EXE-OFF-NEXT:   acc_ev_enqueue_upload_start: var_name=nc.i, bytes=[[#SIZEOF_T_I]]
      // EXE-OFF-NEXT:   acc_ev_enqueue_upload_end: var_name=nc.i, bytes=[[#SIZEOF_T_I]]
      // EXE-OFF-NEXT: acc_ev_enter_data_end
      // EXE-OFF-NEXT: acc_ev_exit_data_start
      // EXE-OFF-NEXT:   acc_ev_enqueue_download_start: var_name=nc.i, bytes=[[#SIZEOF_T_I]]
      // EXE-OFF-NEXT:   acc_ev_enqueue_download_end: var_name=nc.i, bytes=[[#SIZEOF_T_I]]
      // EXE-OFF-NEXT:   acc_ev_delete: var_name=nc.i, bytes=[[#SIZEOF_T_I]]
      // EXE-OFF-NEXT:   acc_ev_free: var_name=nc.i, bytes=[[#SIZEOF_T_I]]
      // EXE-OFF-NEXT: acc_ev_exit_data_end
      #pragma acc parallel num_gangs(1) copy(c.i,nc.i)
      {
        c.i += 1000;
        nc.i += 1000;
      }

      // PRT-NEXT: printf(
      // PRT:      );
      // PRT-NEXT: c.i +=
      // PRT-NEXT: nc.i +=
      //
      //      EXE-NEXT: After second acc parallel:
      // EXE-HOST-NEXT:    c.i=2210, nc.i=1250
      //  EXE-OFF-NEXT:    c.i= 210, nc.i=1250
      printf("After second acc parallel:\n"
             "   c.i=%4d, nc.i=%4d\n",
             c.i, nc.i);
      c.i += 100;
      nc.i += 100;
    } // PRT-NEXT: }
    // EXE-OFF-NEXT: acc_ev_exit_data_start
    // EXE-OFF-NEXT:   acc_ev_enqueue_download_start: var_name=c.i, bytes=[[#SIZEOF_T_I]]
    // EXE-OFF-NEXT:   acc_ev_enqueue_download_end: var_name=c.i, bytes=[[#SIZEOF_T_I]]
    // EXE-OFF-NEXT:   acc_ev_delete: var_name=c.i, bytes=[[#SIZEOF_T_I]]
    // EXE-OFF-NEXT:   acc_ev_free: var_name=c.i, bytes=[[#SIZEOF_T_I]]
    // EXE-OFF-NEXT: acc_ev_exit_data_end

    // PRT-NEXT: printf(
    // PRT:      );
    //
    //      EXE-NEXT: After acc data:
    // EXE-HOST-NEXT:    c.i=2310, nc.i=1350
    //  EXE-OFF-NEXT:    c.i=2010, nc.i=1350
    printf("After acc data:\n"
           "   c.i=%4d, nc.i=%4d\n",
           c.i, nc.i);
  } // PRT-NEXT: }

  return 0;
}
