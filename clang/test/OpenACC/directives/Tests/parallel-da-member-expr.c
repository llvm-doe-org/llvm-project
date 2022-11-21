// Check implicit and explicit data attributes for member expressions on
// "acc parallel".
//
// It's challenging to check that there are no redundant allocations or
// transfers (i.e., for multiple members or the full struct).  Specifically,
// calling acc_is_present or omp_target_is_present or specifying a present
// clause on a directive only works from the host, so it doesn't help for
// checking while in "acc parallel".  We could examine the output of
// LIBOMPTARGET_DEBUG=1, but it only works in debug builds.  Our solution is to
// utilize the OpenACC Profiling Interface, which we usually only exercise in
// the runtime test suite.
//
// The effect of an enclosing "acc data" is checked in "data-da-member-expr.c".

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

struct S {
  int x;
  int y;
  int z;
};

struct SS {
  struct S s;
};

int main() {
  struct S s;
  struct SS ss;

  //      EXE: sizeof s = [[#%d,SIZEOF_S:]]
  // EXE-NEXT: sizeof s.x = [[#%d,SIZEOF_S_X:]]
  // EXE-NEXT: sizeof s.y = [[#%d,SIZEOF_S_Y:]]
  // EXE-NEXT: sizeof s.z = [[#%d,SIZEOF_S_Z:]]
  // EXE-NEXT: sizeof ss = [[#%d,SIZEOF_SS:]]
  // EXE-NEXT: sizeof(void *) = [[#%d,SIZEOF_PTR:]]
  printf("sizeof s = %zu\n", sizeof s);
  printf("sizeof s.x = %zu\n", sizeof s.x);
  printf("sizeof s.y = %zu\n", sizeof s.y);
  printf("sizeof s.z = %zu\n", sizeof s.z);
  printf("sizeof ss = %zu\n", sizeof ss);
  printf("sizeof(void *) = %zu\n", sizeof(void *));

  //============================================================================
  // Check that using a member of a struct produces an implicit data attribute
  // for the struct.
  //============================================================================

  //----------------------------------------------------------------------------
  // Check simple case.
  //----------------------------------------------------------------------------

  // DMP-LABEL: "implicit da for use of one member\n"
  // PRT-LABEL: "implicit da for use of one member\n"
  // EXE-LABEL: implicit da for use of one member
  printf("implicit da for use of one member\n");

  //      DMP: ACCParallelDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 1
  // DMP-NEXT:   ACCCopyClause {{.*}} <implicit>
  // DMP-NEXT:     DeclRefExpr {{.*}} 's'
  // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  // DMP-NEXT:     DeclRefExpr {{.*}} 's'
  // DMP-NEXT:   impl: OMPTargetTeamsDirective
  // DMP-NEXT:     OMPNum_teamsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 1
  // DMP-NEXT:     OMPMapClause
  //  DMP-NOT:       <implicit>
  // DMP-NEXT:       DeclRefExpr {{.*}} 's'
  //
  //         PRT: s.z = {{.}};
  //  PRT-A-NEXT: #pragma acc parallel num_gangs(1){{$}}
  // PRT-AO-NEXT: // #pragma omp target teams num_teams(1) map(ompx_hold,tofrom: s) shared(s){{$}}
  //  PRT-O-NEXT: #pragma omp target teams num_teams(1) map(ompx_hold,tofrom: s) shared(s){{$}}
  // PRT-OA-NEXT: // #pragma acc parallel num_gangs(1){{$}}
  //
  // EXE-OFF-NEXT: acc_ev_enter_data_start
  // EXE-OFF-NEXT:   acc_ev_alloc: var_name=s, bytes=[[#SIZEOF_S]]
  // EXE-OFF-NEXT:   acc_ev_create: var_name=s, bytes=[[#SIZEOF_S]]
  // EXE-OFF-NEXT:   acc_ev_enqueue_upload_start: var_name=s, bytes=[[#SIZEOF_S]]
  // EXE-OFF-NEXT:   acc_ev_enqueue_upload_end: var_name=s, bytes=[[#SIZEOF_S]]
  // EXE-OFF-NEXT: acc_ev_enter_data_end
  // EXE-OFF-NEXT: acc_ev_exit_data_start
  // EXE-OFF-NEXT:   acc_ev_enqueue_download_start: var_name=s, bytes=[[#SIZEOF_S]]
  // EXE-OFF-NEXT:   acc_ev_enqueue_download_end: var_name=s, bytes=[[#SIZEOF_S]]
  // EXE-OFF-NEXT:   acc_ev_delete: var_name=s, bytes=[[#SIZEOF_S]]
  // EXE-OFF-NEXT:   acc_ev_free: var_name=s, bytes=[[#SIZEOF_S]]
  // EXE-OFF-NEXT: acc_ev_exit_data_end
  //     EXE-NEXT: s.x = 15
  //     EXE-NEXT: s.y = 6
  //     EXE-NEXT: s.z = 7
  s.x = 5;
  s.y = 6;
  s.z = 7;
  #pragma acc parallel num_gangs(1)
  s.x += 10;
  printf("s.x = %d\n", s.x);
  printf("s.y = %d\n", s.y);
  printf("s.z = %d\n", s.z);

  //----------------------------------------------------------------------------
  // Check with uses of multiple members for the same struct.
  //----------------------------------------------------------------------------

  // DMP-LABEL: "implicit da for use of multiple members\n"
  // PRT-LABEL: "implicit da for use of multiple members\n"
  // EXE-LABEL: implicit da for use of multiple members
  printf("implicit da for use of multiple members\n");

  //      DMP: ACCParallelDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 1
  // DMP-NEXT:   ACCCopyClause {{.*}} <implicit>
  // DMP-NEXT:     DeclRefExpr {{.*}} 's'
  // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  // DMP-NEXT:     DeclRefExpr {{.*}} 's'
  // DMP-NEXT:   impl: OMPTargetTeamsDirective
  // DMP-NEXT:     OMPNum_teamsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 1
  // DMP-NEXT:     OMPMapClause
  //  DMP-NOT:       <implicit>
  // DMP-NEXT:       DeclRefExpr {{.*}} 's'
  //
  //         PRT: s.z = {{.}};
  //  PRT-A-NEXT: #pragma acc parallel num_gangs(1){{$}}
  // PRT-AO-NEXT: // #pragma omp target teams num_teams(1) map(ompx_hold,tofrom: s) shared(s){{$}}
  //  PRT-O-NEXT: #pragma omp target teams num_teams(1) map(ompx_hold,tofrom: s) shared(s){{$}}
  // PRT-OA-NEXT: // #pragma acc parallel num_gangs(1){{$}}
  //
  // EXE-OFF-NEXT: acc_ev_enter_data_start
  // EXE-OFF-NEXT:   acc_ev_alloc: var_name=s, bytes=[[#SIZEOF_S]]
  // EXE-OFF-NEXT:   acc_ev_create: var_name=s, bytes=[[#SIZEOF_S]]
  // EXE-OFF-NEXT:   acc_ev_enqueue_upload_start: var_name=s, bytes=[[#SIZEOF_S]]
  // EXE-OFF-NEXT:   acc_ev_enqueue_upload_end: var_name=s, bytes=[[#SIZEOF_S]]
  // EXE-OFF-NEXT: acc_ev_enter_data_end
  // EXE-OFF-NEXT: acc_ev_exit_data_start
  // EXE-OFF-NEXT:   acc_ev_enqueue_download_start: var_name=s, bytes=[[#SIZEOF_S]]
  // EXE-OFF-NEXT:   acc_ev_enqueue_download_end: var_name=s, bytes=[[#SIZEOF_S]]
  // EXE-OFF-NEXT:   acc_ev_delete: var_name=s, bytes=[[#SIZEOF_S]]
  // EXE-OFF-NEXT:   acc_ev_free: var_name=s, bytes=[[#SIZEOF_S]]
  // EXE-OFF-NEXT: acc_ev_exit_data_end
  //     EXE-NEXT: s.x = 15
  //     EXE-NEXT: s.y = 6
  //     EXE-NEXT: s.z = 27
  s.x = 5;
  s.y = 6;
  s.z = 7;
  #pragma acc parallel num_gangs(1)
  {
    s.x += 10;
    s.z += 20;
  }
  printf("s.x = %d\n", s.x);
  printf("s.y = %d\n", s.y);
  printf("s.z = %d\n", s.z);

  //----------------------------------------------------------------------------
  // Check that a loop construct enclosing the member use doesn't prevent the
  // parallel construct from seeing the member use.
  //----------------------------------------------------------------------------

  // DMP-LABEL: "implicit da for member use in loop\n"
  // PRT-LABEL: "implicit da for member use in loop\n"
  // EXE-LABEL: implicit da for member use in loop
  printf("implicit da for member use in loop\n");

  //      DMP: ACCParallelDirective
  // DMP-NEXT:   ACCCopyClause {{.*}} <implicit>
  // DMP-NEXT:     DeclRefExpr {{.*}} 's'
  // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  // DMP-NEXT:     DeclRefExpr {{.*}} 's'
  // DMP-NEXT:   impl: OMPTargetTeamsDirective
  // DMP-NEXT:     OMPMapClause
  //  DMP-NOT:       <implicit>
  // DMP-NEXT:       DeclRefExpr {{.*}} 's'
  //      DMP:   ACCLoopDirective
  //
  //         PRT: s.z = {{.}};
  //  PRT-A-NEXT: #pragma acc parallel{{$}}
  // PRT-AO-NEXT: // #pragma omp target teams map(ompx_hold,tofrom: s) shared(s){{$}}
  //  PRT-A-NEXT: #pragma acc loop{{$}}
  // PRT-AO-NEXT: // #pragma omp distribute
  //  PRT-O-NEXT: #pragma omp target teams map(ompx_hold,tofrom: s) shared(s){{$}}
  // PRT-OA-NEXT: // #pragma acc parallel{{$}}
  //  PRT-O-NEXT: #pragma omp distribute
  // PRT-OA-NEXT: // #pragma acc loop{{$}}
  //
  // EXE-OFF-NEXT: acc_ev_enter_data_start
  // EXE-OFF-NEXT:   acc_ev_alloc: var_name=s, bytes=[[#SIZEOF_S]]
  // EXE-OFF-NEXT:   acc_ev_create: var_name=s, bytes=[[#SIZEOF_S]]
  // EXE-OFF-NEXT:   acc_ev_enqueue_upload_start: var_name=s, bytes=[[#SIZEOF_S]]
  // EXE-OFF-NEXT:   acc_ev_enqueue_upload_end: var_name=s, bytes=[[#SIZEOF_S]]
  // EXE-OFF-NEXT: acc_ev_enter_data_end
  // EXE-OFF-NEXT: acc_ev_exit_data_start
  // EXE-OFF-NEXT:   acc_ev_enqueue_download_start: var_name=s, bytes=[[#SIZEOF_S]]
  // EXE-OFF-NEXT:   acc_ev_enqueue_download_end: var_name=s, bytes=[[#SIZEOF_S]]
  // EXE-OFF-NEXT:   acc_ev_delete: var_name=s, bytes=[[#SIZEOF_S]]
  // EXE-OFF-NEXT:   acc_ev_free: var_name=s, bytes=[[#SIZEOF_S]]
  // EXE-OFF-NEXT: acc_ev_exit_data_end
  //     EXE-NEXT: s.x = 15
  //     EXE-NEXT: s.y = 6
  //     EXE-NEXT: s.z = 7
  s.x = 5;
  s.y = 6;
  s.z = 7;
  #pragma acc parallel
  #pragma acc loop
  for (int i = 0; i < 1; ++i)
    s.x += 10;
  printf("s.x = %d\n", s.x);
  printf("s.y = %d\n", s.y);
  printf("s.z = %d\n", s.z);

  //----------------------------------------------------------------------------
  // Check that being part of a combined construct doesn't prevent the effective
  // parallel construct from seeing the member use.
  //----------------------------------------------------------------------------

  // DMP-LABEL: "implicit da for member use in combined construct\n"
  // PRT-LABEL: "implicit da for member use in combined construct\n"
  // EXE-LABEL: implicit da for member use in combined construct
  printf("implicit da for member use in combined construct\n");

  //      DMP: ACCParallelLoopDirective
  // DMP-NEXT:   effect: ACCParallelDirective
  // DMP-NEXT:     ACCCopyClause {{.*}} <implicit>
  // DMP-NEXT:       DeclRefExpr {{.*}} 's'
  // DMP-NEXT:     ACCSharedClause {{.*}} <implicit>
  // DMP-NEXT:       DeclRefExpr {{.*}} 's'
  // DMP-NEXT:     impl: OMPTargetTeamsDirective
  // DMP-NEXT:       OMPMapClause
  //  DMP-NOT:         <implicit>
  // DMP-NEXT:         DeclRefExpr {{.*}} 's'
  //      DMP:     ACCLoopDirective
  //
  //         PRT: s.z = {{.}};
  //  PRT-A-NEXT: #pragma acc parallel loop{{$}}
  // PRT-AO-NEXT: // #pragma omp target teams map(ompx_hold,tofrom: s) shared(s){{$}}
  // PRT-AO-NEXT: // #pragma omp distribute
  //  PRT-O-NEXT: #pragma omp target teams map(ompx_hold,tofrom: s) shared(s){{$}}
  //  PRT-O-NEXT: #pragma omp distribute
  // PRT-OA-NEXT: // #pragma acc parallel loop{{$}}
  //
  // EXE-OFF-NEXT: acc_ev_enter_data_start
  // EXE-OFF-NEXT:   acc_ev_alloc: var_name=s, bytes=[[#SIZEOF_S]]
  // EXE-OFF-NEXT:   acc_ev_create: var_name=s, bytes=[[#SIZEOF_S]]
  // EXE-OFF-NEXT:   acc_ev_enqueue_upload_start: var_name=s, bytes=[[#SIZEOF_S]]
  // EXE-OFF-NEXT:   acc_ev_enqueue_upload_end: var_name=s, bytes=[[#SIZEOF_S]]
  // EXE-OFF-NEXT: acc_ev_enter_data_end
  // EXE-OFF-NEXT: acc_ev_exit_data_start
  // EXE-OFF-NEXT:   acc_ev_enqueue_download_start: var_name=s, bytes=[[#SIZEOF_S]]
  // EXE-OFF-NEXT:   acc_ev_enqueue_download_end: var_name=s, bytes=[[#SIZEOF_S]]
  // EXE-OFF-NEXT:   acc_ev_delete: var_name=s, bytes=[[#SIZEOF_S]]
  // EXE-OFF-NEXT:   acc_ev_free: var_name=s, bytes=[[#SIZEOF_S]]
  // EXE-OFF-NEXT: acc_ev_exit_data_end
  //     EXE-NEXT: s.x = 15
  //     EXE-NEXT: s.y = 6
  //     EXE-NEXT: s.z = 7
  s.x = 5;
  s.y = 6;
  s.z = 7;
  #pragma acc parallel loop
  for (int i = 0; i < 1; ++i)
    s.x += 10;
  printf("s.x = %d\n", s.x);
  printf("s.y = %d\n", s.y);
  printf("s.z = %d\n", s.z);

  //----------------------------------------------------------------------------
  // Check that using a member of a member of a struct produces an implicit data
  // attribute for the outermost struct.
  //
  // This also checks that an implicit data attribute is computed successfully
  // for a used member expression that would not have been permitted in a data
  // clause (because, in this example, it's a member of a member).  That is, it
  // checks that clang doesn't malfunction trying to look for an explicit data
  // attribute for that member expression.
  //----------------------------------------------------------------------------

  // DMP-LABEL: "implicit da for member of member use\n"
  // PRT-LABEL: "implicit da for member of member use\n"
  // EXE-LABEL: implicit da for member of member use
  printf("implicit da for member of member use\n");

  //      DMP: ACCParallelDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 1
  // DMP-NEXT:   ACCCopyClause {{.*}} <implicit>
  // DMP-NEXT:     DeclRefExpr {{.*}} 'ss'
  // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  // DMP-NEXT:     DeclRefExpr {{.*}} 'ss'
  // DMP-NEXT:   impl: OMPTargetTeamsDirective
  // DMP-NEXT:     OMPNum_teamsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 1
  // DMP-NEXT:     OMPMapClause
  //  DMP-NOT:       <implicit>
  // DMP-NEXT:       DeclRefExpr {{.*}} 'ss'
  //
  //         PRT: ss.s.z = {{.}};
  //  PRT-A-NEXT: #pragma acc parallel num_gangs(1){{$}}
  // PRT-AO-NEXT: // #pragma omp target teams num_teams(1) map(ompx_hold,tofrom: ss) shared(ss){{$}}
  //  PRT-O-NEXT: #pragma omp target teams num_teams(1) map(ompx_hold,tofrom: ss) shared(ss){{$}}
  // PRT-OA-NEXT: // #pragma acc parallel num_gangs(1){{$}}
  //
  // EXE-OFF-NEXT: acc_ev_enter_data_start
  // EXE-OFF-NEXT:   acc_ev_alloc: var_name=ss, bytes=[[#SIZEOF_SS]]
  // EXE-OFF-NEXT:   acc_ev_create: var_name=ss, bytes=[[#SIZEOF_SS]]
  // EXE-OFF-NEXT:   acc_ev_enqueue_upload_start: var_name=ss, bytes=[[#SIZEOF_SS]]
  // EXE-OFF-NEXT:   acc_ev_enqueue_upload_end: var_name=ss, bytes=[[#SIZEOF_SS]]
  // EXE-OFF-NEXT: acc_ev_enter_data_end
  // EXE-OFF-NEXT: acc_ev_exit_data_start
  // EXE-OFF-NEXT:   acc_ev_enqueue_download_start: var_name=ss, bytes=[[#SIZEOF_SS]]
  // EXE-OFF-NEXT:   acc_ev_enqueue_download_end: var_name=ss, bytes=[[#SIZEOF_SS]]
  // EXE-OFF-NEXT:   acc_ev_delete: var_name=ss, bytes=[[#SIZEOF_SS]]
  // EXE-OFF-NEXT:   acc_ev_free: var_name=ss, bytes=[[#SIZEOF_SS]]
  // EXE-OFF-NEXT: acc_ev_exit_data_end
  //     EXE-NEXT: ss.s.x = 5
  //     EXE-NEXT: ss.s.y = 16
  //     EXE-NEXT: ss.s.z = 7
  ss.s.x = 5;
  ss.s.y = 6;
  ss.s.z = 7;
  #pragma acc parallel num_gangs(1)
  ss.s.y += 10;
  printf("ss.s.x = %d\n", ss.s.x);
  printf("ss.s.y = %d\n", ss.s.y);
  printf("ss.s.z = %d\n", ss.s.z);

  //----------------------------------------------------------------------------
  // Check with -> operator.
  //
  // FIXME: firstprivate(ptr) currently misbehaves in Clang's OpenMP
  // implementation.  It appears to have been fixed more recently in upstream,
  // so the following EXE-HOST-NEXT will then become EXE-NEXT, and the
  // associated EXE-OFF-NEXT will be removed.
  //----------------------------------------------------------------------------

  // DMP-LABEL: "implicit da for use of member via ->\n"
  // PRT-LABEL: "implicit da for use of member via ->\n"
  // EXE-LABEL: implicit da for use of member via ->
  printf("implicit da for use of member via ->\n");

  //      DMP: ACCParallelDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 1
  // DMP-NEXT:   ACCCopyClause
  // DMP-NEXT:     DeclRefExpr {{.*}} 'py'
  // DMP-NEXT:   ACCNomapClause {{.*}} <implicit>
  // DMP-NEXT:     DeclRefExpr {{.*}} 'ps'
  // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  // DMP-NEXT:     DeclRefExpr {{.*}} 'py'
  // DMP-NEXT:   ACCFirstprivateClause {{.*}} <implicit>
  // DMP-NEXT:     DeclRefExpr {{.*}} 'ps'
  // DMP-NEXT:   impl: OMPTargetTeamsDirective
  // DMP-NEXT:     OMPNum_teamsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 1
  // DMP-NEXT:     OMPMapClause
  //  DMP-NOT:       <implicit>
  // DMP-NEXT:       DeclRefExpr {{.*}} 'py'
  // DMP-NEXT:     OMPSharedClause
  //  DMP-NOT:       <implicit>
  // DMP-NEXT:       DeclRefExpr {{.*}} 'py'
  // DMP-NEXT:     OMPFirstprivateClause
  //  DMP-NOT:       <implicit>
  // DMP-NEXT:       DeclRefExpr {{.*}} 'ps'
  //
  //         PRT: int *py;
  //  PRT-A-NEXT: #pragma acc parallel num_gangs(1) copy(py){{$}}
  // PRT-AO-NEXT: // #pragma omp target teams num_teams(1) map(ompx_hold,tofrom: py) shared(py) firstprivate(ps){{$}}
  //  PRT-O-NEXT: #pragma omp target teams num_teams(1) map(ompx_hold,tofrom: py) shared(py) firstprivate(ps){{$}}
  // PRT-OA-NEXT: // #pragma acc parallel num_gangs(1) copy(py){{$}}
  //
  //      EXE-NEXT: &s.y = 0x[[#%x,S_Y_ADDR:]]
  //  EXE-OFF-NEXT: acc_ev_enter_data_start
  //  EXE-OFF-NEXT:   acc_ev_alloc: var_name=py, bytes=[[#SIZEOF_PTR]]
  //  EXE-OFF-NEXT:   acc_ev_create: var_name=py, bytes=[[#SIZEOF_PTR]]
  //  EXE-OFF-NEXT:   acc_ev_enqueue_upload_start: var_name=py, bytes=[[#SIZEOF_PTR]]
  //  EXE-OFF-NEXT:   acc_ev_enqueue_upload_end: var_name=py, bytes=[[#SIZEOF_PTR]]
  //  EXE-OFF-NEXT: acc_ev_enter_data_end
  //  EXE-OFF-NEXT: acc_ev_exit_data_start
  //  EXE-OFF-NEXT:   acc_ev_enqueue_download_start: var_name=py, bytes=[[#SIZEOF_PTR]]
  //  EXE-OFF-NEXT:   acc_ev_enqueue_download_end: var_name=py, bytes=[[#SIZEOF_PTR]]
  //  EXE-OFF-NEXT:   acc_ev_delete: var_name=py, bytes=[[#SIZEOF_PTR]]
  //  EXE-OFF-NEXT:   acc_ev_free: var_name=py, bytes=[[#SIZEOF_PTR]]
  //  EXE-OFF-NEXT: acc_ev_exit_data_end
  // EXE-HOST-NEXT: py = 0x[[#S_Y_ADDR]]
  //  EXE-OFF-NEXT: py = 0x[[#%x,]]
  printf("&s.y = %p\n", &s.y);
  {
    struct S *ps = &s;
    int *py;
    #pragma acc parallel num_gangs(1) copy(py)
    py = &ps->y;
    printf("py = %p\n", py);
  }

  //============================================================================
  // Check that we don't have an implicit data attribute for an entire struct
  // when an existing explicit data attribute for its used members is
  // sufficient.
  //
  // Also check that we don't generate member expressions in OpenMP shared
  // clauses, which Clang doesn't support.
  //============================================================================

  //----------------------------------------------------------------------------
  // Check simple case.
  //----------------------------------------------------------------------------

  // DMP-LABEL: "explicit da for use of one member\n"
  // PRT-LABEL: "explicit da for use of one member\n"
  // EXE-LABEL: explicit da for use of one member
  printf("explicit da for use of one member\n");

  //      DMP: ACCParallelDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 1
  // DMP-NEXT:   ACCCopyClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:     MemberExpr {{.* .y .*}}
  // DMP-NEXT:       DeclRefExpr {{.*}} 's'
  // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  // DMP-NEXT:     MemberExpr {{.* .y .*}}
  // DMP-NEXT:       DeclRefExpr {{.*}} 's'
  // DMP-NEXT:   impl: OMPTargetTeamsDirective
  // DMP-NEXT:     OMPNum_teamsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 1
  // DMP-NEXT:     OMPMapClause
  //  DMP-NOT:       <implicit>
  // DMP-NEXT:       MemberExpr {{.* .y .*}}
  // DMP-NEXT:         DeclRefExpr {{.*}} 's'
  //
  //         PRT: s.z = {{.}};
  //  PRT-A-NEXT: #pragma acc parallel num_gangs(1) copy(s.y){{$}}
  // PRT-AO-NEXT: // #pragma omp target teams num_teams(1) map(ompx_hold,tofrom: s.y){{$}}
  //  PRT-O-NEXT: #pragma omp target teams num_teams(1) map(ompx_hold,tofrom: s.y){{$}}
  // PRT-OA-NEXT: // #pragma acc parallel num_gangs(1) copy(s.y){{$}}
  //
  // EXE-OFF-NEXT: acc_ev_enter_data_start
  // EXE-OFF-NEXT:   acc_ev_alloc: var_name=s.y, bytes=[[#SIZEOF_S_Y]]
  // EXE-OFF-NEXT:   acc_ev_create: var_name=s.y, bytes=[[#SIZEOF_S_Y]]
  // EXE-OFF-NEXT:   acc_ev_enqueue_upload_start: var_name=s.y, bytes=[[#SIZEOF_S_Y]]
  // EXE-OFF-NEXT:   acc_ev_enqueue_upload_end: var_name=s.y, bytes=[[#SIZEOF_S_Y]]
  // EXE-OFF-NEXT: acc_ev_enter_data_end
  // EXE-OFF-NEXT: acc_ev_exit_data_start
  // EXE-OFF-NEXT:   acc_ev_enqueue_download_start: var_name=s.y, bytes=[[#SIZEOF_S_Y]]
  // EXE-OFF-NEXT:   acc_ev_enqueue_download_end: var_name=s.y, bytes=[[#SIZEOF_S_Y]]
  // EXE-OFF-NEXT:   acc_ev_delete: var_name=s.y, bytes=[[#SIZEOF_S_Y]]
  // EXE-OFF-NEXT:   acc_ev_free: var_name=s.y, bytes=[[#SIZEOF_S_Y]]
  // EXE-OFF-NEXT: acc_ev_exit_data_end
  //     EXE-NEXT: s.x = 5
  //     EXE-NEXT: s.y = 26
  //     EXE-NEXT: s.z = 7
  s.x = 5;
  s.y = 6;
  s.z = 7;
  #pragma acc parallel num_gangs(1) copy(s.y)
  s.y += 20;
  printf("s.x = %d\n", s.x);
  printf("s.y = %d\n", s.y);
  printf("s.z = %d\n", s.z);

  //----------------------------------------------------------------------------
  // Check with uses of multiple consecutive members for the same struct.
  //----------------------------------------------------------------------------

  // DMP-LABEL: "explicit da for uses of multiple consecutive members\n"
  // PRT-LABEL: "explicit da for uses of multiple consecutive members\n"
  // EXE-LABEL: explicit da for uses of multiple consecutive members
  printf("explicit da for uses of multiple consecutive members\n");

  //      DMP: ACCParallelDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 1
  // DMP-NEXT:   ACCCopyClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:     MemberExpr {{.* .x .*}}
  // DMP-NEXT:       DeclRefExpr {{.*}} 's'
  // DMP-NEXT:   ACCCopyinClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:     MemberExpr {{.* .y .*}}
  // DMP-NEXT:       DeclRefExpr {{.*}} 's'
  // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  // DMP-NEXT:     MemberExpr {{.* .x .*}}
  // DMP-NEXT:       DeclRefExpr {{.*}} 's'
  // DMP-NEXT:     MemberExpr {{.* .y .*}}
  // DMP-NEXT:       DeclRefExpr {{.*}} 's'
  // DMP-NEXT:   impl: OMPTargetTeamsDirective
  // DMP-NEXT:     OMPNum_teamsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 1
  // DMP-NEXT:     OMPMapClause
  //  DMP-NOT:       <implicit>
  // DMP-NEXT:       MemberExpr {{.* .x .*}}
  // DMP-NEXT:         DeclRefExpr {{.*}} 's'
  // DMP-NEXT:     OMPMapClause
  //  DMP-NOT:       <implicit>
  // DMP-NEXT:       MemberExpr {{.* .y .*}}
  // DMP-NEXT:         DeclRefExpr {{.*}} 's'
  //
  //         PRT: s.z = {{.}};
  //  PRT-A-NEXT: #pragma acc parallel num_gangs(1) copy(s.x) copyin(s.y){{$}}
  // PRT-AO-NEXT: // #pragma omp target teams num_teams(1) map(ompx_hold,tofrom: s.x) map(ompx_hold,to: s.y){{$}}
  //  PRT-O-NEXT: #pragma omp target teams num_teams(1) map(ompx_hold,tofrom: s.x) map(ompx_hold,to: s.y){{$}}
  // PRT-OA-NEXT: // #pragma acc parallel num_gangs(1) copy(s.x) copyin(s.y){{$}}
  //
  //  EXE-OFF-NEXT: acc_ev_enter_data_start
  //  EXE-OFF-NEXT:   acc_ev_alloc: var_name=unknown, bytes=[[#SIZEOF_S_X + SIZEOF_S_Y]]
  //  EXE-OFF-NEXT:   acc_ev_create: var_name=unknown, bytes=[[#SIZEOF_S_X + SIZEOF_S_Y]]
  //  EXE-OFF-NEXT:   acc_ev_enqueue_upload_start: var_name=s.x, bytes=[[#SIZEOF_S_X]]
  //  EXE-OFF-NEXT:   acc_ev_enqueue_upload_end: var_name=s.x, bytes=[[#SIZEOF_S_X]]
  //  EXE-OFF-NEXT:   acc_ev_enqueue_upload_start: var_name=s.y, bytes=[[#SIZEOF_S_Y]]
  //  EXE-OFF-NEXT:   acc_ev_enqueue_upload_end: var_name=s.y, bytes=[[#SIZEOF_S_Y]]
  //  EXE-OFF-NEXT: acc_ev_enter_data_end
  //  EXE-OFF-NEXT: acc_ev_exit_data_start
  //  EXE-OFF-NEXT:   acc_ev_enqueue_download_start: var_name=s.x, bytes=[[#SIZEOF_S_X]]
  //  EXE-OFF-NEXT:   acc_ev_enqueue_download_end: var_name=s.x, bytes=[[#SIZEOF_S_X]]
  //  EXE-OFF-NEXT:   acc_ev_delete: var_name=unknown, bytes=[[#SIZEOF_S_X + SIZEOF_S_Y]]
  //  EXE-OFF-NEXT:   acc_ev_free: var_name=unknown, bytes=[[#SIZEOF_S_X + SIZEOF_S_Y]]
  //  EXE-OFF-NEXT: acc_ev_exit_data_end
  //      EXE-NEXT: s.x = 15
  // EXE-HOST-NEXT: s.y = 26
  //  EXE-OFF-NEXT: s.y = 6
  //      EXE-NEXT: s.z = 7
  s.x = 5;
  s.y = 6;
  s.z = 7;
  #pragma acc parallel num_gangs(1) copy(s.x) copyin(s.y)
  {
    s.x += 10;
    s.y += 20;
  }
  printf("s.x = %d\n", s.x);
  printf("s.y = %d\n", s.y);
  printf("s.z = %d\n", s.z);

  //----------------------------------------------------------------------------
  // Check with uses of multiple non-consecutive members for the same struct.
  //----------------------------------------------------------------------------

  // DMP-LABEL: "explicit da for uses of multiple non-consecutive members\n"
  // PRT-LABEL: "explicit da for uses of multiple non-consecutive members\n"
  // EXE-LABEL: explicit da for uses of multiple non-consecutive members
  printf("explicit da for uses of multiple non-consecutive members\n");

  //      DMP: ACCParallelDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 1
  // DMP-NEXT:   ACCCopyinClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:     MemberExpr {{.* .x .*}}
  // DMP-NEXT:       DeclRefExpr {{.*}} 's'
  // DMP-NEXT:   ACCCopyClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:     MemberExpr {{.* .z .*}}
  // DMP-NEXT:       DeclRefExpr {{.*}} 's'
  // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  // DMP-NEXT:     MemberExpr {{.* .x .*}}
  // DMP-NEXT:       DeclRefExpr {{.*}} 's'
  // DMP-NEXT:     MemberExpr {{.* .z .*}}
  // DMP-NEXT:       DeclRefExpr {{.*}} 's'
  // DMP-NEXT:   impl: OMPTargetTeamsDirective
  // DMP-NEXT:     OMPNum_teamsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 1
  // DMP-NEXT:     OMPMapClause
  //  DMP-NOT:       <implicit>
  // DMP-NEXT:       MemberExpr {{.* .x .*}}
  // DMP-NEXT:         DeclRefExpr {{.*}} 's'
  // DMP-NEXT:     OMPMapClause
  //  DMP-NOT:       <implicit>
  // DMP-NEXT:       MemberExpr {{.* .z .*}}
  // DMP-NEXT:         DeclRefExpr {{.*}} 's'
  //
  //         PRT: s.z = {{.}};
  //  PRT-A-NEXT: #pragma acc parallel num_gangs(1) copyin(s.x) copy(s.z){{$}}
  // PRT-AO-NEXT: // #pragma omp target teams num_teams(1) map(ompx_hold,to: s.x) map(ompx_hold,tofrom: s.z){{$}}
  //  PRT-O-NEXT: #pragma omp target teams num_teams(1) map(ompx_hold,to: s.x) map(ompx_hold,tofrom: s.z){{$}}
  // PRT-OA-NEXT: // #pragma acc parallel num_gangs(1) copyin(s.x) copy(s.z){{$}}
  //
  //  EXE-OFF-NEXT: acc_ev_enter_data_start
  //  EXE-OFF-NEXT:   acc_ev_alloc: var_name=unknown, bytes=[[#SIZEOF_S_X + SIZEOF_S_Y + SIZEOF_S_Z]]
  //  EXE-OFF-NEXT:   acc_ev_create: var_name=unknown, bytes=[[#SIZEOF_S_X + SIZEOF_S_Y + SIZEOF_S_Z]]
  //  EXE-OFF-NEXT:   acc_ev_enqueue_upload_start: var_name=s.x, bytes=[[#SIZEOF_S_X]]
  //  EXE-OFF-NEXT:   acc_ev_enqueue_upload_end: var_name=s.x, bytes=[[#SIZEOF_S_X]]
  //  EXE-OFF-NEXT:   acc_ev_enqueue_upload_start: var_name=s.z, bytes=[[#SIZEOF_S_Z]]
  //  EXE-OFF-NEXT:   acc_ev_enqueue_upload_end: var_name=s.z, bytes=[[#SIZEOF_S_Z]]
  //  EXE-OFF-NEXT: acc_ev_enter_data_end
  //  EXE-OFF-NEXT: acc_ev_exit_data_start
  //  EXE-OFF-NEXT:   acc_ev_enqueue_download_start: var_name=s.z, bytes=[[#SIZEOF_S_Z]]
  //  EXE-OFF-NEXT:   acc_ev_enqueue_download_end: var_name=s.z, bytes=[[#SIZEOF_S_Z]]
  //  EXE-OFF-NEXT:   acc_ev_delete: var_name=unknown, bytes=[[#SIZEOF_S_X + SIZEOF_S_Y + SIZEOF_S_Z]]
  //  EXE-OFF-NEXT:   acc_ev_free: var_name=unknown, bytes=[[#SIZEOF_S_X + SIZEOF_S_Y + SIZEOF_S_Z]]
  //  EXE-OFF-NEXT: acc_ev_exit_data_end
  // EXE-HOST-NEXT: s.x = 15
  //  EXE-OFF-NEXT: s.x = 5
  //      EXE-NEXT: s.y = 6
  //      EXE-NEXT: s.z = 37
  s.x = 5;
  s.y = 6;
  s.z = 7;
  #pragma acc parallel num_gangs(1) copyin(s.x) copy(s.z)
  {
    s.x += 10;
    s.z += 30;
  }
  printf("s.x = %d\n", s.x);
  printf("s.y = %d\n", s.y);
  printf("s.z = %d\n", s.z);

  //----------------------------------------------------------------------------
  // Check with -> operator.
  //----------------------------------------------------------------------------

  // DMP-LABEL: "explicit da for use of member via ->\n"
  // PRT-LABEL: "explicit da for use of member via ->\n"
  // EXE-LABEL: explicit da for use of member via ->
  printf("explicit da for use of member via ->\n");

  //      DMP: ACCParallelDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 1
  // DMP-NEXT:   ACCCopyClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:     MemberExpr {{.* ->z .*}}
  // DMP-NEXT:       ImplicitCastExpr
  // DMP-NEXT:         DeclRefExpr {{.*}} 'ps'
  // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  // DMP-NEXT:     MemberExpr {{.* ->z .*}}
  // DMP-NEXT:       ImplicitCastExpr
  // DMP-NEXT:         DeclRefExpr {{.*}} 'ps'
  // DMP-NEXT:   impl: OMPTargetTeamsDirective
  // DMP-NEXT:     OMPNum_teamsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 1
  // DMP-NEXT:     OMPMapClause
  //  DMP-NOT:       <implicit>
  // DMP-NEXT:       MemberExpr {{.* ->z .*}}
  // DMP-NEXT:         ImplicitCastExpr
  // DMP-NEXT:           DeclRefExpr {{.*}} 'ps'
  //
  //         PRT: struct S *ps = &s;
  //  PRT-A-NEXT: #pragma acc parallel num_gangs(1) copy(ps->z){{$}}
  // PRT-AO-NEXT: // #pragma omp target teams num_teams(1) map(ompx_hold,tofrom: ps->z){{$}}
  //  PRT-O-NEXT: #pragma omp target teams num_teams(1) map(ompx_hold,tofrom: ps->z){{$}}
  // PRT-OA-NEXT: // #pragma acc parallel num_gangs(1) copy(ps->z){{$}}
  //
  // EXE-OFF-NEXT: acc_ev_enter_data_start
  // EXE-OFF-NEXT:   acc_ev_alloc: var_name=ps->z, bytes=[[#SIZEOF_S_Z]]
  // EXE-OFF-NEXT:   acc_ev_create: var_name=ps->z, bytes=[[#SIZEOF_S_Z]]
  // EXE-OFF-NEXT:   acc_ev_enqueue_upload_start: var_name=ps->z, bytes=[[#SIZEOF_S_Z]]
  // EXE-OFF-NEXT:   acc_ev_enqueue_upload_end: var_name=ps->z, bytes=[[#SIZEOF_S_Z]]
  // EXE-OFF-NEXT: acc_ev_enter_data_end
  // EXE-OFF-NEXT: acc_ev_exit_data_start
  // EXE-OFF-NEXT:   acc_ev_enqueue_download_start: var_name=ps->z, bytes=[[#SIZEOF_S_Z]]
  // EXE-OFF-NEXT:   acc_ev_enqueue_download_end: var_name=ps->z, bytes=[[#SIZEOF_S_Z]]
  // EXE-OFF-NEXT:   acc_ev_delete: var_name=ps->z, bytes=[[#SIZEOF_S_Z]]
  // EXE-OFF-NEXT:   acc_ev_free: var_name=ps->z, bytes=[[#SIZEOF_S_Z]]
  // EXE-OFF-NEXT: acc_ev_exit_data_end
  //     EXE-NEXT: s.x = 5
  //     EXE-NEXT: s.y = 6
  //     EXE-NEXT: s.z = 37
  s.x = 5;
  s.y = 6;
  s.z = 7;
  {
    struct S *ps = &s;
    #pragma acc parallel num_gangs(1) copy(ps->z)
    ps->z += 30;
  }
  printf("s.x = %d\n", s.x);
  printf("s.y = %d\n", s.y);
  printf("s.z = %d\n", s.z);

  return 0;
}
