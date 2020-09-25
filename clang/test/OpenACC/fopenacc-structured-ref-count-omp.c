// Check the effect of -fopenacc-structured-ref-count-omp on the translation of
// data clauses, and check the effect on the presence of data at run time.
//
// In various other tests for specific directives and clauses, various other
// dimensions of data clause behavior are checked thoroughly when 'hold' is
// used in the translation, and we don't bother to repeat those checks when
// 'hold' isn't used.  Diagnostics about 'hold' in the translation are checked
// in warn-acc-omp-map-hold.c.
//
// In some cases, it's challenging to check when data manages to remain present.
// Specifically, calling acc_is_present or omp_target_is_present or specifying a
// present clause on a directive only works from the host, so it doesn't help
// when checking within "acc parallel".  We could examine the output of
// LIBOMPTARGET_DEBUG=1, but it only works in debug builds.  Our solution is to
// utilize the OpenACC Profiling Interface, which we usually only exercise in
// the runtime test suite.
//
// TODO: Once "acc exit data" is implemented, extend this test to be sure that
// cannot remove data when 'hold' is used but can otherwise.

// Check bad -fopenacc-structured-ref-count-omp values.
//
// RUN: %data bad-vals {
// RUN:   (val=foo)
// RUN:   (val=   )
// RUN: }
// RUN: %data bad-vals-cmds {
// RUN:   (cmd='%clang -fopenacc'    )
// RUN:   (cmd='%clang_cc1 -fopenacc')
// RUN:   (cmd='%clang'              )
// RUN:   (cmd='%clang_cc1'          )
// RUN: }
// RUN: %for bad-vals {
// RUN:   %for bad-vals-cmds {
// RUN:     not %[cmd] -fopenacc-structured-ref-count-omp=%[val] %s 2>&1 \
// RUN:     | FileCheck -check-prefix=BAD-VAL -DVAL=%[val] %s
// RUN:   }
// RUN: }
//
// BAD-VAL: error: invalid value '[[VAL]]' in '-fopenacc-structured-ref-count-omp=[[VAL]]'

// RUN: %data ref-count-opts {
// RUN:   (ref-count-opt=-Wno-openacc-omp-map-hold                                           hold='hold,')
// RUN:   (ref-count-opt='-fopenacc-structured-ref-count-omp=hold -Wno-openacc-omp-map-hold' hold='hold,')
// RUN:   (ref-count-opt=-fopenacc-structured-ref-count-omp=no-hold                          hold=       )
// RUN: }

// Check -ast-dump before and after AST serialization.
//
// RUN: %for ref-count-opts {
// RUN:   %clang -Xclang -verify -Xclang -ast-dump -fsyntax-only -fopenacc %s \
// RUN:          %acc_includes %[ref-count-opt] \
// RUN:   | FileCheck -check-prefixes=DMP %s
// RUN:   %clang -Xclang -verify -fopenacc -emit-ast -o %t.ast %s \
// RUN:          %acc_includes %[ref-count-opt]
// RUN:   %clang_cc1 -ast-dump-all %t.ast \
// RUN:   | FileCheck -check-prefixes=DMP %s
// RUN: }

// Check -ast-print and -fopenacc[-ast]-print.
//
// TODO: If lit were to support %for inside a %data, we could iterate prt-opts
// within prt-args after the first prt-args iteration, significantly shortening
// the prt-args definition.
//
// Strip comments and blank lines so checking -fopenacc-print output is easier.
// RUN: echo "// expected""-no-diagnostics" > %t-acc.c
// RUN: grep -v '^ *\(//.*\)\?$' %s | sed 's,//.*,,' >> %t-acc.c
//
// RUN: %data prt-opts {
// RUN:   (prt-opt=-fopenacc-ast-print)
// RUN:   (prt-opt=-fopenacc-print    )
// RUN: }
// RUN: %data prt-args {
// RUN:   (prt='-Xclang -ast-print -fsyntax-only -fopenacc' prt-chk=PRT,PRT-A       )
// RUN:   (prt=-fopenacc-ast-print=acc                      prt-chk=PRT,PRT-A       )
// RUN:   (prt=-fopenacc-ast-print=omp                      prt-chk=PRT,PRT-O       )
// RUN:   (prt=-fopenacc-ast-print=acc-omp                  prt-chk=PRT,PRT-A,PRT-AO)
// RUN:   (prt=-fopenacc-ast-print=omp-acc                  prt-chk=PRT,PRT-O,PRT-OA)
// RUN:   (prt=-fopenacc-print=acc                          prt-chk=PRT,PRT-A       )
// RUN:   (prt=-fopenacc-print=omp                          prt-chk=PRT,PRT-O       )
// RUN:   (prt=-fopenacc-print=acc-omp                      prt-chk=PRT,PRT-A,PRT-AO)
// RUN:   (prt=-fopenacc-print=omp-acc                      prt-chk=PRT,PRT-O,PRT-OA)
// RUN: }
// RUN: %for ref-count-opts {
// RUN:   %for prt-args {
// RUN:     %clang -Xclang -verify %[prt] %[ref-count-opt] %t-acc.c \
// RUN:            %acc_includes -Wno-openacc-omp-map-present \
// RUN:            -Wno-openacc-omp-map-no-alloc \
// RUN:     | FileCheck -check-prefixes=%[prt-chk] -DHOLD='%[hold]' %s
// RUN:   }
// RUN: }

// Check -ast-print after AST serialization.
//
// Some data related to printing (where to print comments about discarded
// directives) is serialized and deserialized, so it's worthwhile to try all
// OpenACC printing modes.
//
// RUN: %for ref-count-opts {
// RUN:   %clang -Xclang -verify -fopenacc %[ref-count-opt] -emit-ast %t-acc.c \
// RUN:          -o %t.ast %acc_includes
// RUN:   %for prt-args {
// RUN:     %clang %[prt] %t.ast 2>&1 \
// RUN:     | FileCheck -check-prefixes=%[prt-chk] -DHOLD='%[hold]' %s
// RUN:   }
// RUN: }

// Can we print the OpenMP source code, compile, and run it successfully?
//
// We don't normally bother to check this for offloading, but DMAs have no
// effect when not offloading (that is, for shared memory), and one of the main
// issues here is the various ways the data clauses can be translated so they
// can be used in source-to-source when targeting other compilers.  That is, we
// want to be sure source-to-source mode produces working translations of the
// data clauses in all cases.
//
// RUN: %data tgts {
// RUN:   (run-if=                tgt-cflags=                                     host-or-dev=HOST)
// RUN:   (run-if=%run-if-x86_64  tgt-cflags=-fopenmp-targets=%run-x86_64-triple  host-or-dev=DEV )
// RUN:   (run-if=%run-if-ppc64le tgt-cflags=-fopenmp-targets=%run-ppc64le-triple host-or-dev=DEV )
// RUN:   (run-if=%run-if-nvptx64 tgt-cflags=-fopenmp-targets=%run-nvptx64-triple host-or-dev=DEV )
// RUN: }
// RUN: %for ref-count-opts {
// RUN:   %for tgts {
// RUN:     %for prt-opts {
// RUN:       %[run-if] %clang -Xclang -verify %[prt-opt]=omp %[ref-count-opt] \
// RUN:                        %s %acc_includes > %t-omp.c \
// RUN:                        -Wno-openacc-omp-map-present \
// RUN:                        -Wno-openacc-omp-map-no-alloc
// RUN:       %[run-if] echo "// expected""-no-diagnostics" >> %t-omp.c
// RUN:       %[run-if] %clang -Xclang -verify -fopenmp %fopenmp-version \
// RUN:                 %[tgt-cflags] -o %t.exe %t-omp.c %acc_includes
// RUN:       %[run-if] %t.exe > %t.out 2>&1
// RUN:       %[run-if] FileCheck -input-file %t.out %s \
// RUN:         -match-full-lines -allow-empty \
// RUN:         -check-prefixes=EXE,EXE-%[host-or-dev]
// RUN:     }
// RUN:   }
// RUN: }

// Check execution with normal compilation.
//
// RUN: %for ref-count-opts {
// RUN:   %for tgts {
// RUN:     %[run-if] %clang -Xclang -verify -fopenacc %[ref-count-opt] \
// RUN:               %[tgt-cflags] -o %t.exe %s %acc_includes
// RUN:     %[run-if] %t.exe > %t.out 2>&1
// RUN:     %[run-if] FileCheck -input-file %t.out %s \
// RUN:       -match-full-lines -allow-empty \
// RUN:       -check-prefixes=EXE,EXE-%[host-or-dev]
// RUN:   }
// RUN: }

// END.

// expected-no-diagnostics

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
  // EXE: start
  printf("start\n");

  //--------------------------------------------------
  // Initially absent.
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
  //   DMP-NOT: -{{OMP|ACC}}
  //
  //  PRT-A-NEXT: #pragma acc data copy(c) copyin(ci) copyout(co) create(cr)
  //  PRT-A-SAME: {{^}} no_create(nc){{$}}
  // PRT-AO-NEXT: // #pragma omp target data map([[HOLD]]tofrom: c)
  // PRT-AO-SAME: {{^}} map([[HOLD]]to: ci) map([[HOLD]]from: co)
  // PRT-AO-SAME: {{^}} map([[HOLD]]alloc: cr)
  // PRT-AO-SAME: {{^}} map(no_alloc,[[HOLD]]alloc: nc){{$}}
  //  PRT-O-NEXT: #pragma omp target data map([[HOLD]]tofrom: c)
  //  PRT-O-SAME: {{^}} map([[HOLD]]to: ci) map([[HOLD]]from: co)
  //  PRT-O-SAME: {{^}} map([[HOLD]]alloc: cr)
  //  PRT-O-SAME: {{^}} map(no_alloc,[[HOLD]]alloc: nc){{$}}
  // PRT-OA-NEXT: // #pragma acc data copy(c) copyin(ci) copyout(co)
  // PRT-OA-SAME: {{^}} create(cr) no_create(nc){{$}}
  //
  // EXE-DEV-NEXT: acc_ev_enter_data_start
  // EXE-DEV-NEXT: acc_ev_alloc  var_name={{<null>|c}}
  // EXE-DEV-NEXT: acc_ev_create var_name={{<null>|c}}
  // EXE-DEV-NEXT: acc_ev_alloc  var_name={{<null>|ci}}
  // EXE-DEV-NEXT: acc_ev_create var_name={{<null>|ci}}
  // EXE-DEV-NEXT: acc_ev_alloc  var_name={{<null>|co}}
  // EXE-DEV-NEXT: acc_ev_create var_name={{<null>|co}}
  // EXE-DEV-NEXT: acc_ev_alloc  var_name={{<null>|cr}}
  // EXE-DEV-NEXT: acc_ev_create var_name={{<null>|cr}}
  // EXE-DEV-NEXT: acc_ev_enter_data_end
  #pragma acc data copy(c) copyin(ci) copyout(co) create(cr) no_create(nc)
  // PRT: ;
  ;
  // EXE-DEV-NEXT: acc_ev_exit_data_start
  // EXE-DEV-NEXT: acc_ev_delete var_name={{<null>|cr}}
  // EXE-DEV-NEXT: acc_ev_free   var_name={{<null>|cr}}
  // EXE-DEV-NEXT: acc_ev_delete var_name={{<null>|co}}
  // EXE-DEV-NEXT: acc_ev_free   var_name={{<null>|co}}
  // EXE-DEV-NEXT: acc_ev_delete var_name={{<null>|ci}}
  // EXE-DEV-NEXT: acc_ev_free   var_name={{<null>|ci}}
  // EXE-DEV-NEXT: acc_ev_delete var_name={{<null>|c}}
  // EXE-DEV-NEXT: acc_ev_free   var_name={{<null>|c}}
  // EXE-DEV-NEXT: acc_ev_exit_data_end

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
  // PRT-AO-SAME: {{^}} map(no_alloc,[[HOLD]]alloc: nc){{$}}
  //  PRT-O-NEXT: #pragma omp target teams map([[HOLD]]tofrom: c)
  //  PRT-O-SAME: {{^}} map([[HOLD]]to: ci) map([[HOLD]]from: co)
  //  PRT-O-SAME: {{^}} map([[HOLD]]alloc: cr)
  //  PRT-O-SAME: {{^}} map(no_alloc,[[HOLD]]alloc: nc){{$}}
  // PRT-OA-NEXT: // #pragma acc parallel copy(c) copyin(ci) copyout(co)
  // PRT-OA-SAME: {{^}} create(cr) no_create(nc){{$}}
  //
  // EXE-DEV-NEXT: acc_ev_enter_data_start
  // EXE-DEV-NEXT: acc_ev_alloc  var_name={{<null>|c}}
  // EXE-DEV-NEXT: acc_ev_create var_name={{<null>|c}}
  // EXE-DEV-NEXT: acc_ev_alloc  var_name={{<null>|ci}}
  // EXE-DEV-NEXT: acc_ev_create var_name={{<null>|ci}}
  // EXE-DEV-NEXT: acc_ev_alloc  var_name={{<null>|co}}
  // EXE-DEV-NEXT: acc_ev_create var_name={{<null>|co}}
  // EXE-DEV-NEXT: acc_ev_alloc  var_name={{<null>|cr}}
  // EXE-DEV-NEXT: acc_ev_create var_name={{<null>|cr}}
  // EXE-DEV-NEXT: acc_ev_enter_data_end
  #pragma acc parallel copy(c) copyin(ci) copyout(co) create(cr) no_create(nc)
  // PRT: ;
  ;
  // EXE-DEV-NEXT: acc_ev_exit_data_start
  // EXE-DEV-NEXT: acc_ev_delete var_name={{<null>|cr}}
  // EXE-DEV-NEXT: acc_ev_free   var_name={{<null>|cr}}
  // EXE-DEV-NEXT: acc_ev_delete var_name={{<null>|co}}
  // EXE-DEV-NEXT: acc_ev_free   var_name={{<null>|co}}
  // EXE-DEV-NEXT: acc_ev_delete var_name={{<null>|ci}}
  // EXE-DEV-NEXT: acc_ev_free   var_name={{<null>|ci}}
  // EXE-DEV-NEXT: acc_ev_delete var_name={{<null>|c}}
  // EXE-DEV-NEXT: acc_ev_free   var_name={{<null>|c}}
  // EXE-DEV-NEXT: acc_ev_exit_data_end

  //--------------------------------------------------
  // Initially present.
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
  // EXE-DEV-NEXT: acc_ev_enter_data_start
  // EXE-DEV-NEXT: acc_ev_alloc  var_name={{<null>|p}}
  // EXE-DEV-NEXT: acc_ev_create var_name={{<null>|p}}
  // EXE-DEV-NEXT: acc_ev_alloc  var_name={{<null>|nc}}
  // EXE-DEV-NEXT: acc_ev_create var_name={{<null>|nc}}
  // EXE-DEV-NEXT: acc_ev_enter_data_end
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
    //   DMP-NOT: -{{OMP|ACC}}
    //
    //  PRT-A-NEXT: #pragma acc data present(p) no_create(nc){{$}}
    // PRT-AO-NEXT: // #pragma omp target data map(present,[[HOLD]]alloc: p)
    // PRT-AO-SAME: {{^}} map(no_alloc,[[HOLD]]alloc: nc){{$}}
    //  PRT-O-NEXT: #pragma omp target data map(present,[[HOLD]]alloc: p)
    //  PRT-O-SAME: {{^}} map(no_alloc,[[HOLD]]alloc: nc){{$}}
    // PRT-OA-NEXT: // #pragma acc data present(p) no_create(nc){{$}}
    // EXE-DEV-NEXT: acc_ev_enter_data_start
    // EXE-DEV-NEXT: acc_ev_enter_data_end
    #pragma acc data present(p) no_create(nc)
    // PRT-NEXT: ;
    ;
    // EXE-DEV-NEXT: acc_ev_exit_data_start
    // EXE-DEV-NEXT: acc_ev_exit_data_end

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
    // PRT-AO-SAME: {{^}} map(no_alloc,[[HOLD]]alloc: nc){{$}}
    //  PRT-O-NEXT: #pragma omp target teams map(present,[[HOLD]]alloc: p)
    //  PRT-O-SAME: {{^}} map(no_alloc,[[HOLD]]alloc: nc){{$}}
    // PRT-OA-NEXT: // #pragma acc parallel present(p) no_create(nc){{$}}
    //
    // EXE-DEV-NEXT: acc_ev_enter_data_start
    // EXE-DEV-NEXT: acc_ev_enter_data_end
    #pragma acc parallel present(p) no_create(nc)
    // PRT-NEXT: ;
    ;
    // EXE-DEV-NEXT: acc_ev_exit_data_start
    // EXE-DEV-NEXT: acc_ev_exit_data_end
  } // PRT-NEXT: }
  // EXE-DEV-NEXT: acc_ev_exit_data_start
  // EXE-DEV-NEXT: acc_ev_delete var_name={{<null>|nc}}
  // EXE-DEV-NEXT: acc_ev_free   var_name={{<null>|nc}}
  // EXE-DEV-NEXT: acc_ev_delete var_name={{<null>|p}}
  // EXE-DEV-NEXT: acc_ev_free   var_name={{<null>|p}}
  // EXE-DEV-NEXT: acc_ev_exit_data_end

  // PRT-NEXT: return 0;
  return 0;
} // PRT-NEXT: }

// EXE-NOT: {{.}}
