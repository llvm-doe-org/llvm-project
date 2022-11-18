// Check the effect of -fopenacc-structured-ref-count-omp on the translation of
// data clauses, and check the effect on the presence of data at run time, in
// particular when acc exit data is called more times than acc enter data.
//
// In various other tests for specific directives and clauses, various other
// dimensions of data clause behavior are checked thoroughly when 'ompx_hold' is
// used in the translation, and we don't bother to repeat those checks when
// 'ompx_hold' isn't used.  Diagnostics about 'ompx_hold' in the translation are
// checked in diagnostics/warn-acc-omp-map-ompx-hold.c.  Diagnostics about bad
// -fopenacc-structured-ref-count-omp arguments are checked in
// diagnostics/fopenacc-structured-ref-count-omp.c.
//
// In some cases, it's challenging to check when data manages to remain present.
// Specifically, calling acc_is_present or omp_target_is_present or specifying a
// present clause on a directive only works from the host, so it doesn't help
// when checking within "acc parallel".  We could examine the output of
// LIBOMPTARGET_DEBUG=1, but it only works in debug builds.  Our solution is to
// utilize the OpenACC Profiling Interface, which we usually only exercise in
// the runtime test suite.

// REDEFINE: %{all:clang:args-stable} = -gline-tables-only
// REDEFINE: %{exe:fc:args-stable} = -match-full-lines -allow-empty

// REDEFINE: %{all:clang:args} =
// REDEFINE: %{prt:fc:args} = -DHOLD='ompx_hold,'
// REDEFINE: %{exe:fc:pres} = HOLD
// RUN: %{acc-check-dmp}
// RUN: %{acc-check-prt}
// RUN: %{acc-check-exe}

// REDEFINE: %{all:clang:args} = -fopenacc-structured-ref-count-omp=ompx-hold
// RUN: %{acc-check-dmp}
// RUN: %{acc-check-prt}
// RUN: %{acc-check-exe}

// REDEFINE: %{all:clang:args} = -fopenacc-structured-ref-count-omp=no-ompx-hold
// REDEFINE: %{prt:fc:args} = -DHOLD=
// REDEFINE: %{exe:fc:pres} = NO-HOLD
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
    printf(" var_name=%s", ei->data_event.var_name ? ei->data_event.var_name
                                                   : "<null>");
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
}

// EXE-NOT: {{.}}

// PRT: int main() {
int main() {
  // PRT-NEXT: int p
  int p, c, ci, co, cr, nc;

  // PRT-NEXT: printf
  // EXE:start
  printf("start\n");

  //--------------------------------------------------
  // acc data when initially absent.
  //--------------------------------------------------

  // DMP-LABEL: ACCDataDirective
  //  DMP-NEXT:   ACCCopyClause
  //  DMP-NEXT:     DeclRefExpr {{.*}} 'c' 'int'
  //  DMP-NEXT:   ACCCopyinClause
  //  DMP-NEXT:     DeclRefExpr {{.*}} 'ci' 'int'
  //  DMP-NEXT:   ACCCopyoutClause
  //  DMP-NEXT:     DeclRefExpr {{.*}} 'co' 'int'
  //  DMP-NEXT:   ACCCreateClause
  //  DMP-NEXT:     DeclRefExpr {{.*}} 'cr' 'int'
  //  DMP-NEXT:   ACCNoCreateClause
  //  DMP-NEXT:     DeclRefExpr {{.*}} 'nc' 'int'
  //  DMP-NEXT:   impl: OMPTargetDataDirective
  //  DMP-NEXT:     OMPMapClause
  //  DMP-NEXT:       DeclRefExpr {{.*}} 'c' 'int'
  //  DMP-NEXT:     OMPMapClause
  //  DMP-NEXT:       DeclRefExpr {{.*}} 'ci' 'int'
  //  DMP-NEXT:     OMPMapClause
  //  DMP-NEXT:       DeclRefExpr {{.*}} 'co' 'int'
  //  DMP-NEXT:     OMPMapClause
  //  DMP-NEXT:       DeclRefExpr {{.*}} 'cr' 'int'
  //  DMP-NEXT:     OMPMapClause
  //  DMP-NEXT:       DeclRefExpr {{.*}} 'nc' 'int'
  //   DMP-NOT: -{{ACC}}
  //
  //  PRT-A-NEXT: #pragma acc data copy(c) copyin(ci) copyout(co) create(cr)
  //  PRT-A-SAME: {{^}} no_create(nc){{$}}
  // PRT-AO-NEXT: // #pragma omp target data map([[HOLD]]tofrom: c)
  // PRT-AO-SAME: {{^}} map([[HOLD]]to: ci) map([[HOLD]]from: co)
  // PRT-AO-SAME: {{^}} map([[HOLD]]alloc: cr)
  // PRT-AO-SAME: {{^}} map(ompx_no_alloc,[[HOLD]]alloc: nc){{$}}
  //  PRT-O-NEXT: #pragma omp target data map([[HOLD]]tofrom: c)
  //  PRT-O-SAME: {{^}} map([[HOLD]]to: ci) map([[HOLD]]from: co)
  //  PRT-O-SAME: {{^}} map([[HOLD]]alloc: cr)
  //  PRT-O-SAME: {{^}} map(ompx_no_alloc,[[HOLD]]alloc: nc){{$}}
  // PRT-OA-NEXT: // #pragma acc data copy(c) copyin(ci) copyout(co)
  // PRT-OA-SAME: {{^}} create(cr) no_create(nc){{$}}
  //
  // EXE-OFF-NEXT:acc_ev_enter_data_start
  // EXE-OFF-NEXT:acc_ev_alloc var_name={{c}}
  // EXE-OFF-NEXT:acc_ev_create var_name={{c}}
  // EXE-OFF-NEXT:acc_ev_alloc var_name={{ci}}
  // EXE-OFF-NEXT:acc_ev_create var_name={{ci}}
  // EXE-OFF-NEXT:acc_ev_alloc var_name={{co}}
  // EXE-OFF-NEXT:acc_ev_create var_name={{co}}
  // EXE-OFF-NEXT:acc_ev_alloc var_name={{cr}}
  // EXE-OFF-NEXT:acc_ev_create var_name={{cr}}
  // EXE-OFF-NEXT:acc_ev_enter_data_end
  #pragma acc data copy(c) copyin(ci) copyout(co) create(cr) no_create(nc)
  // PRT-NEXT: {
  {
    // DMP: ACCExitDataDirective
    //
    //  PRT-A-NEXT: #pragma acc exit data copyout(c,ci,co) delete(cr){{$}}
    // PRT-AO-NEXT: // #pragma omp target exit data map(from: c,ci,co) map(release: cr){{$}}
    //  PRT-O-NEXT: #pragma omp target exit data map(from: c,ci,co) map(release: cr){{$}}
    // PRT-OA-NEXT: // #pragma acc exit data copyout(c,ci,co) delete(cr){{$}}
    //
    //         EXE-OFF-NEXT:acc_ev_exit_data_start
    // EXE-OFF-NO-HOLD-NEXT:acc_ev_delete var_name={{cr}}
    // EXE-OFF-NO-HOLD-NEXT:acc_ev_free var_name={{cr}}
    // EXE-OFF-NO-HOLD-NEXT:acc_ev_delete var_name={{co}}
    // EXE-OFF-NO-HOLD-NEXT:acc_ev_free var_name={{co}}
    // EXE-OFF-NO-HOLD-NEXT:acc_ev_delete var_name={{ci}}
    // EXE-OFF-NO-HOLD-NEXT:acc_ev_free var_name={{ci}}
    // EXE-OFF-NO-HOLD-NEXT:acc_ev_delete var_name={{c}}
    // EXE-OFF-NO-HOLD-NEXT:acc_ev_free var_name={{c}}
    //         EXE-OFF-NEXT:acc_ev_exit_data_end
    #pragma acc exit data copyout(c,ci,co) delete(cr)
  } // PRT-NEXT: }
  //      EXE-OFF-NEXT:acc_ev_exit_data_start
  // EXE-OFF-HOLD-NEXT:acc_ev_delete var_name={{cr}}
  // EXE-OFF-HOLD-NEXT:acc_ev_free var_name={{cr}}
  // EXE-OFF-HOLD-NEXT:acc_ev_delete var_name={{co}}
  // EXE-OFF-HOLD-NEXT:acc_ev_free var_name={{co}}
  // EXE-OFF-HOLD-NEXT:acc_ev_delete var_name={{ci}}
  // EXE-OFF-HOLD-NEXT:acc_ev_free var_name={{ci}}
  // EXE-OFF-HOLD-NEXT:acc_ev_delete var_name={{c}}
  // EXE-OFF-HOLD-NEXT:acc_ev_free var_name={{c}}
  //      EXE-OFF-NEXT:acc_ev_exit_data_end

  //--------------------------------------------------
  // acc parallel when initially absent.
  //--------------------------------------------------

  // DMP-LABEL: ACCParallelDirective
  //  DMP-NEXT:   ACCCopyClause
  //  DMP-NEXT:     DeclRefExpr {{.*}} 'c' 'int'
  //  DMP-NEXT:   ACCCopyinClause
  //  DMP-NEXT:     DeclRefExpr {{.*}} 'ci' 'int'
  //  DMP-NEXT:   ACCCopyoutClause
  //  DMP-NEXT:     DeclRefExpr {{.*}} 'co' 'int'
  //  DMP-NEXT:   ACCCreateClause
  //  DMP-NEXT:     DeclRefExpr {{.*}} 'cr' 'int'
  //  DMP-NEXT:   ACCNoCreateClause
  //  DMP-NEXT:     DeclRefExpr {{.*}} 'nc' 'int'
  //  DMP-NEXT:   impl: OMPTargetTeamsDirective
  //  DMP-NEXT:     OMPMapClause
  //  DMP-NEXT:       DeclRefExpr {{.*}} 'c' 'int'
  //  DMP-NEXT:     OMPMapClause
  //  DMP-NEXT:       DeclRefExpr {{.*}} 'ci' 'int'
  //  DMP-NEXT:     OMPMapClause
  //  DMP-NEXT:       DeclRefExpr {{.*}} 'co' 'int'
  //  DMP-NEXT:     OMPMapClause
  //  DMP-NEXT:       DeclRefExpr {{.*}} 'cr' 'int'
  //  DMP-NEXT:     OMPMapClause
  //  DMP-NEXT:       DeclRefExpr {{.*}} 'nc' 'int'
  //   DMP-NOT: -{{OMP|ACC}}
  //
  //  PRT-A-NEXT: #pragma acc parallel copy(c) copyin(ci) copyout(co) create(cr)
  //  PRT-A-SAME: {{^}} no_create(nc){{$}}
  // PRT-AO-NEXT: // #pragma omp target teams map([[HOLD]]tofrom: c)
  // PRT-AO-SAME: {{^}} map([[HOLD]]to: ci) map([[HOLD]]from: co)
  // PRT-AO-SAME: {{^}} map([[HOLD]]alloc: cr)
  // PRT-AO-SAME: {{^}} map(ompx_no_alloc,[[HOLD]]alloc: nc){{$}}
  //  PRT-O-NEXT: #pragma omp target teams map([[HOLD]]tofrom: c)
  //  PRT-O-SAME: {{^}} map([[HOLD]]to: ci) map([[HOLD]]from: co)
  //  PRT-O-SAME: {{^}} map([[HOLD]]alloc: cr)
  //  PRT-O-SAME: {{^}} map(ompx_no_alloc,[[HOLD]]alloc: nc){{$}}
  // PRT-OA-NEXT: // #pragma acc parallel copy(c) copyin(ci) copyout(co)
  // PRT-OA-SAME: {{^}} create(cr) no_create(nc){{$}}
  //
  // EXE-OFF-NEXT:acc_ev_enter_data_start
  // EXE-OFF-NEXT:acc_ev_alloc var_name={{c}}
  // EXE-OFF-NEXT:acc_ev_create var_name={{c}}
  // EXE-OFF-NEXT:acc_ev_alloc var_name={{ci}}
  // EXE-OFF-NEXT:acc_ev_create var_name={{ci}}
  // EXE-OFF-NEXT:acc_ev_alloc var_name={{co}}
  // EXE-OFF-NEXT:acc_ev_create var_name={{co}}
  // EXE-OFF-NEXT:acc_ev_alloc var_name={{cr}}
  // EXE-OFF-NEXT:acc_ev_create var_name={{cr}}
  // EXE-OFF-NEXT:acc_ev_enter_data_end
  #pragma acc parallel copy(c) copyin(ci) copyout(co) create(cr) no_create(nc)
  // PRT: ;
  ;
  // EXE-OFF-NEXT:acc_ev_exit_data_start
  // EXE-OFF-NEXT:acc_ev_delete var_name={{cr}}
  // EXE-OFF-NEXT:acc_ev_free var_name={{cr}}
  // EXE-OFF-NEXT:acc_ev_delete var_name={{co}}
  // EXE-OFF-NEXT:acc_ev_free var_name={{co}}
  // EXE-OFF-NEXT:acc_ev_delete var_name={{ci}}
  // EXE-OFF-NEXT:acc_ev_free var_name={{ci}}
  // EXE-OFF-NEXT:acc_ev_delete var_name={{c}}
  // EXE-OFF-NEXT:acc_ev_free var_name={{c}}
  // EXE-OFF-NEXT:acc_ev_exit_data_end

  //--------------------------------------------------
  // acc data when initially present (enclosing acc data).
  //--------------------------------------------------

  // Check acc data around acc data.

  // DMP-LABEL: ACCDataDirective
  //  DMP-NEXT:   ACCCreateClause
  //  DMP-NEXT:     DeclRefExpr {{.*}} 'p' 'int'
  //  DMP-NEXT:     DeclRefExpr {{.*}} 'nc' 'int'
  //  DMP-NEXT:   impl: OMPTargetDataDirective
  //  DMP-NEXT:     OMPMapClause
  //  DMP-NEXT:       DeclRefExpr {{.*}} 'p' 'int'
  //  DMP-NEXT:       DeclRefExpr {{.*}} 'nc' 'int'
  //   DMP-NOT: -{{ACC}}
  //
  //  PRT-A-NEXT: #pragma acc data create(p,nc){{$}}
  // PRT-AO-NEXT: // #pragma omp target data map([[HOLD]]alloc: p,nc){{$}}
  //  PRT-O-NEXT: #pragma omp target data map([[HOLD]]alloc: p,nc){{$}}
  // PRT-OA-NEXT: // #pragma acc data create(p,nc){{$}}
  //
  // EXE-OFF-NEXT:acc_ev_enter_data_start
  // EXE-OFF-NEXT:acc_ev_alloc var_name={{p}}
  // EXE-OFF-NEXT:acc_ev_create var_name={{p}}
  // EXE-OFF-NEXT:acc_ev_alloc var_name={{nc}}
  // EXE-OFF-NEXT:acc_ev_create var_name={{nc}}
  // EXE-OFF-NEXT:acc_ev_enter_data_end
  #pragma acc data create(p,nc)
  // PRT-NEXT: {
  {
    // DMP-LABEL: ACCDataDirective
    //  DMP-NEXT:   ACCPresentClause
    //  DMP-NEXT:     DeclRefExpr {{.*}} 'p' 'int'
    //  DMP-NEXT:   ACCNoCreateClause
    //  DMP-NEXT:     DeclRefExpr {{.*}} 'nc' 'int'
    //  DMP-NEXT:   impl: OMPTargetDataDirective
    //  DMP-NEXT:     OMPMapClause
    //  DMP-NEXT:       DeclRefExpr {{.*}} 'p' 'int'
    //  DMP-NEXT:     OMPMapClause
    //  DMP-NEXT:       DeclRefExpr {{.*}} 'nc' 'int'
    //   DMP-NOT: -{{ACC}}
    //
    //  PRT-A-NEXT: #pragma acc data present(p) no_create(nc){{$}}
    // PRT-AO-NEXT: // #pragma omp target data map(present,[[HOLD]]alloc: p)
    // PRT-AO-SAME: {{^}} map(ompx_no_alloc,[[HOLD]]alloc: nc){{$}}
    //  PRT-O-NEXT: #pragma omp target data map(present,[[HOLD]]alloc: p)
    //  PRT-O-SAME: {{^}} map(ompx_no_alloc,[[HOLD]]alloc: nc){{$}}
    // PRT-OA-NEXT: // #pragma acc data present(p) no_create(nc){{$}}
    // EXE-OFF-NEXT:acc_ev_enter_data_start
    // EXE-OFF-NEXT:acc_ev_enter_data_end
    #pragma acc data present(p) no_create(nc)
    // PRT-NEXT: {
    {
      // DMP: ACCExitDataDirective
      //
      //  PRT-A-NEXT: #pragma acc exit data delete(p) copyout(nc){{$}}
      // PRT-AO-NEXT: // #pragma omp target exit data map(release: p) map(from: nc){{$}}
      //  PRT-O-NEXT: #pragma omp target exit data map(release: p) map(from: nc){{$}}
      // PRT-OA-NEXT: // #pragma acc exit data delete(p) copyout(nc){{$}}
      //
      // EXE-OFF-NEXT:acc_ev_exit_data_start
      // EXE-OFF-NEXT:acc_ev_exit_data_end
      #pragma acc exit data delete(p) copyout(nc)
      // DMP: ACCExitDataDirective
      //
      //  PRT-A-NEXT: #pragma acc exit data copyout(p) delete(nc){{$}}
      // PRT-AO-NEXT: // #pragma omp target exit data map(from: p) map(release: nc){{$}}
      //  PRT-O-NEXT: #pragma omp target exit data map(from: p) map(release: nc){{$}}
      // PRT-OA-NEXT: // #pragma acc exit data copyout(p) delete(nc){{$}}
      //
      //         EXE-OFF-NEXT:acc_ev_exit_data_start
      // EXE-OFF-NO-HOLD-NEXT:acc_ev_delete var_name={{nc}}
      // EXE-OFF-NO-HOLD-NEXT:acc_ev_free var_name={{nc}}
      // EXE-OFF-NO-HOLD-NEXT:acc_ev_delete var_name={{p}}
      // EXE-OFF-NO-HOLD-NEXT:acc_ev_free var_name={{p}}
      //         EXE-OFF-NEXT:acc_ev_exit_data_end
      #pragma acc exit data copyout(p) delete(nc)
    } // PRT-NEXT: }
    // EXE-OFF-NEXT:acc_ev_exit_data_start
    // EXE-OFF-NEXT:acc_ev_exit_data_end
  } // PRT-NEXT: }
  //      EXE-OFF-NEXT:acc_ev_exit_data_start
  // EXE-OFF-HOLD-NEXT:acc_ev_delete var_name={{nc}}
  // EXE-OFF-HOLD-NEXT:acc_ev_free var_name={{nc}}
  // EXE-OFF-HOLD-NEXT:acc_ev_delete var_name={{p}}
  // EXE-OFF-HOLD-NEXT:acc_ev_free var_name={{p}}
  //      EXE-OFF-NEXT:acc_ev_exit_data_end

  //--------------------------------------------------
  // acc parallel when initially present (enclosing acc data).
  //--------------------------------------------------

  // DMP-LABEL: ACCDataDirective
  //  DMP-NEXT:   ACCCreateClause
  //  DMP-NEXT:     DeclRefExpr {{.*}} 'p' 'int'
  //  DMP-NEXT:     DeclRefExpr {{.*}} 'nc' 'int'
  //  DMP-NEXT:   impl: OMPTargetDataDirective
  //  DMP-NEXT:     OMPMapClause
  //  DMP-NEXT:       DeclRefExpr {{.*}} 'p' 'int'
  //  DMP-NEXT:       DeclRefExpr {{.*}} 'nc' 'int'
  //   DMP-NOT: -{{ACC}}
  //
  //  PRT-A-NEXT: #pragma acc data create(p,nc){{$}}
  // PRT-AO-NEXT: // #pragma omp target data map([[HOLD]]alloc: p,nc){{$}}
  //  PRT-O-NEXT: #pragma omp target data map([[HOLD]]alloc: p,nc){{$}}
  // PRT-OA-NEXT: // #pragma acc data create(p,nc){{$}}
  //
  // EXE-OFF-NEXT:acc_ev_enter_data_start
  // EXE-OFF-NEXT:acc_ev_alloc var_name={{p}}
  // EXE-OFF-NEXT:acc_ev_create var_name={{p}}
  // EXE-OFF-NEXT:acc_ev_alloc var_name={{nc}}
  // EXE-OFF-NEXT:acc_ev_create var_name={{nc}}
  // EXE-OFF-NEXT:acc_ev_enter_data_end
  #pragma acc data create(p,nc)
  // PRT-NEXT: {
  {
    // DMP-LABEL: ACCParallelDirective
    //  DMP-NEXT:   ACCPresentClause
    //  DMP-NEXT:     DeclRefExpr {{.*}} 'p' 'int'
    //  DMP-NEXT:   ACCNoCreateClause
    //  DMP-NEXT:     DeclRefExpr {{.*}} 'nc' 'int'
    //  DMP-NEXT:   impl: OMPTargetTeamsDirective
    //  DMP-NEXT:     OMPMapClause
    //  DMP-NEXT:       DeclRefExpr {{.*}} 'p' 'int'
    //  DMP-NEXT:     OMPMapClause
    //  DMP-NEXT:       DeclRefExpr {{.*}} 'nc' 'int'
    //   DMP-NOT: -{{OMP|ACC}}
    //
    //  PRT-A-NEXT: #pragma acc parallel present(p) no_create(nc){{$}}
    // PRT-AO-NEXT: // #pragma omp target teams map(present,[[HOLD]]alloc: p)
    // PRT-AO-SAME: {{^}} map(ompx_no_alloc,[[HOLD]]alloc: nc){{$}}
    //  PRT-O-NEXT: #pragma omp target teams map(present,[[HOLD]]alloc: p)
    //  PRT-O-SAME: {{^}} map(ompx_no_alloc,[[HOLD]]alloc: nc){{$}}
    // PRT-OA-NEXT: // #pragma acc parallel present(p) no_create(nc){{$}}
    //
    // EXE-OFF-NEXT:acc_ev_enter_data_start
    // EXE-OFF-NEXT:acc_ev_enter_data_end
    #pragma acc parallel present(p) no_create(nc)
    // PRT-NEXT: ;
    ;
    // EXE-OFF-NEXT:acc_ev_exit_data_start
    // EXE-OFF-NEXT:acc_ev_exit_data_end
    // PRT-NEXT: {
    {
      // DMP: ACCExitDataDirective
      //
      //  PRT-A-NEXT: #pragma acc exit data copyout(p) delete(nc){{$}}
      // PRT-AO-NEXT: // #pragma omp target exit data map(from: p) map(release: nc){{$}}
      //  PRT-O-NEXT: #pragma omp target exit data map(from: p) map(release: nc){{$}}
      // PRT-OA-NEXT: // #pragma acc exit data copyout(p) delete(nc){{$}}
      //
      //         EXE-OFF-NEXT:acc_ev_exit_data_start
      // EXE-OFF-NO-HOLD-NEXT:acc_ev_delete var_name={{nc}}
      // EXE-OFF-NO-HOLD-NEXT:acc_ev_free var_name={{nc}}
      // EXE-OFF-NO-HOLD-NEXT:acc_ev_delete var_name={{p}}
      // EXE-OFF-NO-HOLD-NEXT:acc_ev_free var_name={{p}}
      //         EXE-OFF-NEXT:acc_ev_exit_data_end
      #pragma acc exit data copyout(p) delete(nc)
    } // PRT-NEXT: }
  } // PRT-NEXT: }
  //      EXE-OFF-NEXT:acc_ev_exit_data_start
  // EXE-OFF-HOLD-NEXT:acc_ev_delete var_name={{nc}}
  // EXE-OFF-HOLD-NEXT:acc_ev_free var_name={{nc}}
  // EXE-OFF-HOLD-NEXT:acc_ev_delete var_name={{p}}
  // EXE-OFF-HOLD-NEXT:acc_ev_free var_name={{p}}
  //      EXE-OFF-NEXT:acc_ev_exit_data_end

  // PRT-NEXT: return 0;
  return 0;
} // PRT-NEXT: }

// EXE-NOT: {{.}}
