// Check "acc update" with different values of -fopenacc-update-present-omp.
//
// Diagnostics about the present motion modifier in the translation are tested
// in warn-acc-omp-update-present.c.  The "if" clause is tested in update-if.c.
//
// The various cases covered here should be kept consistent with present.c and
// no-create.c.  For example, a subarray that extends a subarray already present
// is consistently considered not present, so the present clause produces a
// runtime error and the no_create clause doesn't allocate.

// Check bad -fopenacc-update-present-omp values.
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
// RUN:     not %[cmd] -fopenacc-update-present-omp=%[val] %s 2>&1 \
// RUN:     | FileCheck -check-prefix=BAD-VAL -DVAL=%[val] %s
// RUN:   }
// RUN: }
//
// BAD-VAL: error: invalid value '[[VAL]]' in '-fopenacc-update-present-omp=[[VAL]]'

// Define some interrelated data we use several times below.
//
// RUN: %data present-opts {
// RUN:   (present-opt='-DIF_PRESENT=                                                    -Wno-openacc-omp-update-present' present='present: ' if-present=           not-if-present=not not-crash-if-present='not --crash')
// RUN:   (present-opt='-DIF_PRESENT=           -fopenacc-update-present-omp=present     -Wno-openacc-omp-update-present' present='present: ' if-present=           not-if-present=not not-crash-if-present='not --crash')
// RUN:   (present-opt='-DIF_PRESENT=           -fopenacc-update-present-omp=no-present'                                  present=            if-present=           not-if-present=    not-crash-if-present=             )
// RUN:   (present-opt='-DIF_PRESENT=if_present'                                                                          present=            if-present=IF_PRESENT not-if-present=    not-crash-if-present=             )
// RUN:   (present-opt='-DIF_PRESENT=if_present -fopenacc-update-present-omp=no-present'                                  present=            if-present=IF_PRESENT not-if-present=    not-crash-if-present=             )
// RUN: }
// RUN: %data tgts {
// RUN:   (run-if=                tgt-cflags=                                     host-or-dev=HOST not-if-off-and-present=                  not-crash-if-off-and-present=                        not-if-off=    not-crash-if-off=             )
// RUN:   (run-if=%run-if-x86_64  tgt-cflags=-fopenmp-targets=%run-x86_64-triple  host-or-dev=DEV  not-if-off-and-present=%[not-if-present] not-crash-if-off-and-present=%[not-crash-if-present] not-if-off=not not-crash-if-off='not --crash')
// RUN:   (run-if=%run-if-ppc64le tgt-cflags=-fopenmp-targets=%run-ppc64le-triple host-or-dev=DEV  not-if-off-and-present=%[not-if-present] not-crash-if-off-and-present=%[not-crash-if-present] not-if-off=not not-crash-if-off='not --crash')
// RUN:   (run-if=%run-if-nvptx64 tgt-cflags=-fopenmp-targets=%run-nvptx64-triple host-or-dev=DEV  not-if-off-and-present=%[not-if-present] not-crash-if-off-and-present=%[not-crash-if-present] not-if-off=not not-crash-if-off='not --crash')
// RUN: }
// RUN: %data cases {
// RUN:   (case=CASE_NO_PARENT_PRESENT      not-if-fail=                          not-crash-if-fail=                               )
// RUN:   (case=CASE_NO_PARENT_ABSENT       not-if-fail=%[not-if-off-and-present] not-crash-if-fail=%[not-crash-if-off-and-present])
// RUN:   (case=CASE_SCALAR_PRESENT         not-if-fail=                          not-crash-if-fail=                               )
// RUN:   (case=CASE_SCALAR_ABSENT          not-if-fail=%[not-if-off-and-present] not-crash-if-fail=%[not-crash-if-off-and-present])
// RUN:   (case=CASE_STRUCT_PRESENT         not-if-fail=                          not-crash-if-fail=                               )
// RUN:   (case=CASE_STRUCT_ABSENT          not-if-fail=%[not-if-off-and-present] not-crash-if-fail=%[not-crash-if-off-and-present])
// RUN:   (case=CASE_ARRAY_PRESENT          not-if-fail=                          not-crash-if-fail=                               )
// RUN:   (case=CASE_ARRAY_ABSENT           not-if-fail=%[not-if-off-and-present] not-crash-if-fail=%[not-crash-if-off-and-present])
// RUN:   (case=CASE_SUBARRAY_PRESENT       not-if-fail=                          not-crash-if-fail=                               )
// RUN:   (case=CASE_SUBARRAY_PRESENT2      not-if-fail=                          not-crash-if-fail=                               )
// RUN:   (case=CASE_SUBARRAY_DISJOINT      not-if-fail=%[not-if-off-and-present] not-crash-if-fail=%[not-crash-if-off-and-present])
// RUN:   (case=CASE_SUBARRAY_OVERLAP_START not-if-fail=%[not-if-off-and-present] not-crash-if-fail=%[not-crash-if-off-and-present])
// RUN:   (case=CASE_SUBARRAY_OVERLAP_END   not-if-fail=%[not-if-off-and-present] not-crash-if-fail=%[not-crash-if-off-and-present])
// RUN:   (case=CASE_SUBARRAY_CONCAT2       not-if-fail=%[not-if-off-and-present] not-crash-if-fail=%[not-crash-if-off-and-present])
// RUN: }

// Check -ast-dump before and after AST serialization.
//
// RUN: %for present-opts {
// RUN:   %clang -Xclang -verify -Xclang -ast-dump -fsyntax-only -fopenacc %s \
// RUN:          %[present-opt] \
// RUN:   | FileCheck -check-prefixes=DMP,DMP-%[if-present] %s
// RUN:   %clang -Xclang -verify -fopenacc -emit-ast -o %t.ast %s \
// RUN:          %[present-opt]
// RUN:   %clang_cc1 -ast-dump-all %t.ast \
// RUN:   | FileCheck -check-prefixes=DMP,DMP-%[if-present] %s
// RUN: }

// Check -ast-print and -fopenacc[-ast]-print.
//
// We include print checking on only a few representative cases, which should be
// more than sufficient to show it's working for the update directive.
//
// RUN: %clang -Xclang -verify -Xclang -ast-print -fsyntax-only %s \
// RUN: | FileCheck -check-prefixes=PRT %s
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
// RUN:   (prt='-Xclang -ast-print -fsyntax-only -fopenacc' prt-chk=PRT-A,PRT)
// RUN:   (prt=-fopenacc-ast-print=acc                      prt-chk=PRT-A,PRT)
// RUN:   (prt=-fopenacc-ast-print=omp                      prt-chk=PRT-O,PRT)
// RUN:   (prt=-fopenacc-ast-print=acc-omp                  prt-chk=PRT-A,PRT-AO,PRT)
// RUN:   (prt=-fopenacc-ast-print=omp-acc                  prt-chk=PRT-O,PRT-OA,PRT)
// RUN:   (prt=-fopenacc-print=acc                          prt-chk=PRT-A,PRT)
// RUN:   (prt=-fopenacc-print=omp                          prt-chk=PRT-O,PRT)
// RUN:   (prt=-fopenacc-print=acc-omp                      prt-chk=PRT-A,PRT-AO,PRT)
// RUN:   (prt=-fopenacc-print=omp-acc                      prt-chk=PRT-O,PRT-OA,PRT)
// RUN: }
// RUN: %for present-opts {
// RUN:   %for prt-args {
// RUN:     %clang -Xclang -verify %[prt] %[present-opt] %t-acc.c \
// RUN:            -Wno-openacc-omp-map-hold \
// RUN:     | FileCheck -check-prefixes=%[prt-chk] -DPRESENT='%[present]' %s
// RUN:   }
// RUN: }

// Check -ast-print after AST serialization.
//
// Some data related to printing (where to print comments about discarded
// directives) is serialized and deserialized, so it's worthwhile to try all
// OpenACC printing modes.
//
// RUN: %for present-opts {
// RUN:   %clang -Xclang -verify -fopenacc %[present-opt] -emit-ast %t-acc.c \
// RUN:          -o %t.ast
// RUN:   %for prt-args {
// RUN:     %clang %[prt] %t.ast 2>&1 \
// RUN:     | FileCheck -check-prefixes=%[prt-chk] -DPRESENT='%[present]' %s
// RUN:   }
// RUN: }

// Can we print the OpenMP source code, compile, and run it successfully?
//
// We don't normally bother to check this for offloading, but the update
// directive has no effect when not offloading (that is, for shared memory), and
// one of the main issues with the update directive is the various ways it can
// be translated so it can be used in source-to-source when targeting other
// compilers.  That is, we want to be sure source-to-source mode produces
// working translations of the update directive in all cases.
//
// RUN: %for present-opts {
// RUN:   %for tgts {
// RUN:     %for prt-opts {
// RUN:       %[run-if] %clang -Xclang -verify %[prt-opt]=omp %[present-opt] \
// RUN:                 %s -Wno-openacc-omp-map-hold > %t-omp.c
// RUN:       %[run-if] echo "// expected""-no-diagnostics" >> %t-omp.c
// RUN:       %[run-if] %clang -Xclang -verify -fopenmp %fopenmp-version \
// RUN:                 %[tgt-cflags] -o %t.exe %t-omp.c
// RUN:       %for cases {
// RUN:         %[run-if] %[not-crash-if-fail] %t.exe %[case] > %t.out 2> %t.err
// RUN:         %[run-if] FileCheck -input-file %t.err -allow-empty %s \
// RUN:           -check-prefixes=EXE-ERR,EXE-ERR-%[not-if-fail]PASS
// RUN:         %[run-if] FileCheck -input-file %t.out -allow-empty %s \
// RUN:           -strict-whitespace \
// RUN:           -check-prefixes=EXE-OUT,EXE-OUT-%[case] \
// RUN:           -check-prefixes=EXE-OUT-%[case]-%[not-if-fail]PASS \
// RUN:           -check-prefixes=EXE-OUT-%[case]-%[host-or-dev] \
// RUN:           -check-prefixes=EXE-OUT-%[case]-%[host-or-dev]-%[not-if-fail]PASS
// RUN:       }
// RUN:     }
// RUN:   }
// RUN: }

// Check execution with normal compilation.
//
// RUN: %for present-opts {
// RUN:   %for tgts {
// RUN:     %[run-if] %clang -Xclang -verify -fopenacc %[present-opt] \
// RUN:               %[tgt-cflags] -o %t.exe %s
// RUN:     rm -f %t.actual-cases && touch %t.actual-cases
// RUN:     %for cases {
// RUN:       %[run-if] %[not-crash-if-fail] %t.exe %[case] > %t.out 2> %t.err
// RUN:       %[run-if] FileCheck -input-file %t.err -allow-empty %s \
// RUN:         -check-prefixes=EXE-ERR,EXE-ERR-%[not-if-fail]PASS
// RUN:       %[run-if] FileCheck -input-file %t.out -allow-empty %s \
// RUN:         -strict-whitespace \
// RUN:         -check-prefixes=EXE-OUT,EXE-OUT-%[case] \
// RUN:         -check-prefixes=EXE-OUT-%[case]-%[not-if-fail]PASS \
// RUN:         -check-prefixes=EXE-OUT-%[case]-%[host-or-dev] \
// RUN:         -check-prefixes=EXE-OUT-%[case]-%[host-or-dev]-%[not-if-fail]PASS
// RUN:       echo %[case] >> %t.actual-cases
// RUN:     }
// RUN:   }
// RUN: }

// Make sure %data cases didn't omit any cases defined in the code.
//
// RUN: %t.exe -dump-cases > %t.expected-cases
// RUN: diff -u %t.expected-cases %t.actual-cases >&2

// END.

// expected-no-diagnostics

#include <stdio.h>
#include <string.h>

#define FOREACH_CASE(Macro)          \
  Macro(CASE_NO_PARENT_PRESENT)      \
  Macro(CASE_NO_PARENT_ABSENT)       \
  Macro(CASE_SCALAR_PRESENT)         \
  Macro(CASE_SCALAR_ABSENT)          \
  Macro(CASE_STRUCT_PRESENT)         \
  Macro(CASE_STRUCT_ABSENT)          \
  Macro(CASE_ARRAY_PRESENT)          \
  Macro(CASE_ARRAY_ABSENT)           \
  Macro(CASE_SUBARRAY_PRESENT)       \
  Macro(CASE_SUBARRAY_PRESENT2)      \
  Macro(CASE_SUBARRAY_DISJOINT)      \
  Macro(CASE_SUBARRAY_OVERLAP_START) \
  Macro(CASE_SUBARRAY_OVERLAP_END)   \
  Macro(CASE_SUBARRAY_CONCAT2)

enum Case {
#define AddCase(CaseName) \
  CaseName,
FOREACH_CASE(AddCase)
  CASE_END
#undef AddCase
};

const char *CaseNames[] = {
#define AddCase(CaseName) \
  #CaseName,
FOREACH_CASE(AddCase)
#undef AddCase
};

#define PRINT_VAR_INFO(Var) \
  fprintf(stderr, "addr=%p, size=%ld\n", &(Var), sizeof (Var))

#define PRINT_SUBARRAY_INFO(Arr, Start, Length) \
  fprintf(stderr, "addr=%p, size=%ld\n", &(Arr)[Start], \
          Length * sizeof (Arr[0]))

#define setHostInt(Var, Val)   setHostInt_  (&(Var), Val)
#define setDeviceInt(Var, Val) setDeviceInt_(&(Var), Val)

void setHostInt_(int *p, int v) {
  *p = v;
}

void setDeviceInt_(int *p, int v) {
  #pragma acc parallel num_gangs(1)
  *p = v;
}

#define printHostInt(Var)   printHostInt_  (#Var, &Var)
#define printDeviceInt(Var) printDeviceInt_(#Var, &Var)

void printHostInt_(const char *Name, int *Var) {
  printf("    host %s=%d\n", Name, *Var);
}
void printDeviceInt_(const char *Name, int *Var) {
  printf("  device %s=", Name);
  #pragma acc parallel num_gangs(1)
  printf("%d\n", *Var);
}

// DMP-LABEL: FunctionDecl {{.*}} updateNotNested
// PRT-LABEL: void updateNotNested(
void updateNotNested(int *s, int *h, int *d) {
  //                 DMP: ACCUpdateDirective
  //            DMP-NEXT:   ACCSelfClause
  //            DMP-NEXT:     OMPArraySectionExpr
  //            DMP-NEXT:       ImplicitCastExpr {{.*}} 'int *' <LValueToRValue>
  //            DMP-NEXT:         DeclRefExpr {{.*}} 's' 'int *'
  //            DMP-NEXT:       IntegerLiteral {{.*}} 'int' 0
  //            DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
  //            DMP-NEXT:       <<<NULL>>>
  //            DMP-NEXT:   ACCSelfClause
  //            DMP-NEXT:     OMPArraySectionExpr
  //            DMP-NEXT:       ImplicitCastExpr {{.*}} 'int *' <LValueToRValue>
  //            DMP-NEXT:         DeclRefExpr {{.*}} 'h' 'int *'
  //            DMP-NEXT:       IntegerLiteral {{.*}} 'int' 0
  //            DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
  //            DMP-NEXT:       <<<NULL>>>
  //            DMP-NEXT:   ACCDeviceClause
  //            DMP-NEXT:     OMPArraySectionExpr
  //            DMP-NEXT:       ImplicitCastExpr {{.*}} 'int *' <LValueToRValue>
  //            DMP-NEXT:         DeclRefExpr {{.*}} 'd' 'int *'
  //            DMP-NEXT:       IntegerLiteral {{.*}} 'int' 0
  //            DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
  //            DMP-NEXT:       <<<NULL>>>
  // DMP-IF_PRESENT-NEXT:   ACCIfPresentClause
  //            DMP-NEXT:   impl: OMPTargetUpdateDirective
  //            DMP-NEXT:     OMPFromClause
  //            DMP-NEXT:       OMPArraySectionExpr
  //            DMP-NEXT:         ImplicitCastExpr {{.*}} 'int *' <LValueToRValue>
  //            DMP-NEXT:           DeclRefExpr {{.*}} 's' 'int *'
  //            DMP-NEXT:         IntegerLiteral {{.*}} 'int' 0
  //            DMP-NEXT:         IntegerLiteral {{.*}} 'int' 1
  //            DMP-NEXT:         <<<NULL>>>
  //            DMP-NEXT:     OMPFromClause
  //            DMP-NEXT:       OMPArraySectionExpr
  //            DMP-NEXT:         ImplicitCastExpr {{.*}} 'int *' <LValueToRValue>
  //            DMP-NEXT:           DeclRefExpr {{.*}} 'h' 'int *'
  //            DMP-NEXT:         IntegerLiteral {{.*}} 'int' 0
  //            DMP-NEXT:         IntegerLiteral {{.*}} 'int' 1
  //            DMP-NEXT:         <<<NULL>>>
  //            DMP-NEXT:     OMPToClause
  //            DMP-NEXT:       OMPArraySectionExpr
  //            DMP-NEXT:         ImplicitCastExpr {{.*}} 'int *' <LValueToRValue>
  //            DMP-NEXT:           DeclRefExpr {{.*}} 'd' 'int *'
  //            DMP-NEXT:         IntegerLiteral {{.*}} 'int' 0
  //            DMP-NEXT:         IntegerLiteral {{.*}} 'int' 1
  //            DMP-NEXT:         <<<NULL>>>
  //             DMP-NOT: OMP
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc update self(s[0:1]) host(h[0:1]) device(d[0:1]){{( IF_PRESENT| if_present)?$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target update from([[PRESENT]]s[0:1]) from([[PRESENT]]h[0:1]) to([[PRESENT]]d[0:1]){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target update from([[PRESENT]]s[0:1]) from([[PRESENT]]h[0:1]) to([[PRESENT]]d[0:1]){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc update self(s[0:1]) host(h[0:1]) device(d[0:1]){{( IF_PRESENT| if_present)?$}}
  #pragma acc update self(s[0:1]) host(h[0:1]) device(d[0:1]) IF_PRESENT

  // DMP: CallExpr
  printf("updated\n");
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "expected one argument\n");
    return 1;
  }
  if (!strcmp(argv[1], "-dump-cases")) {
    for (int i = 0; i < CASE_END; ++i)
      printf("%s\n", CaseNames[i]);
    return 0;
  }
  enum Case selectedCase;
  for (selectedCase = 0; selectedCase < CASE_END; ++selectedCase) {
    if (!strcmp(argv[1], CaseNames[selectedCase]))
      break;
  }
  if (selectedCase == CASE_END) {
    fprintf(stderr, "unexpected case: %s\n", argv[1]);
    return 1;
  }

  // EXE-OUT: start
  printf("start\n");
  fflush(stdout);

  // DMP-LABEL: SwitchStmt
  // PRT-LABEL: switch (selectedCase)
  switch (selectedCase) {

  //--------------------------------------------------
  // Check acc update not statically nested within another construct.
  //
  // Check with scalar type via subarray on pointer.
  //--------------------------------------------------

  // DMP-LABEL: CASE_NO_PARENT_PRESENT
  // PRT-LABEL: case CASE_NO_PARENT_PRESENT:
  case CASE_NO_PARENT_PRESENT:
  {
    int s, h, d;
    PRINT_VAR_INFO(s);
    PRINT_VAR_INFO(s);
    #pragma acc data create(s,h,d)
    {
      setHostInt(s, 10);
      setHostInt(h, 20);
      setHostInt(d, 30);
      setDeviceInt(s, 11);
      setDeviceInt(h, 21);
      setDeviceInt(d, 31);

      // EXE-OUT-CASE_NO_PARENT_PRESENT-NEXT: updated
      updateNotNested(&s, &h, &d);

      //      EXE-OUT-CASE_NO_PARENT_PRESENT-NEXT:   host s=11{{$}}
      //      EXE-OUT-CASE_NO_PARENT_PRESENT-NEXT:   host h=21{{$}}
      //  EXE-OUT-CASE_NO_PARENT_PRESENT-DEV-NEXT:   host d=30{{$}}
      // EXE-OUT-CASE_NO_PARENT_PRESENT-HOST-NEXT:   host d=31{{$}}
      //      EXE-OUT-CASE_NO_PARENT_PRESENT-NEXT: device s=11{{$}}
      //      EXE-OUT-CASE_NO_PARENT_PRESENT-NEXT: device h=21{{$}}
      //  EXE-OUT-CASE_NO_PARENT_PRESENT-DEV-NEXT: device d=30{{$}}
      // EXE-OUT-CASE_NO_PARENT_PRESENT-HOST-NEXT: device d=31{{$}}
      printHostInt(s);
      printHostInt(h);
      printHostInt(d);
      printDeviceInt(s);
      printDeviceInt(h);
      printDeviceInt(d);
    }
    break;
  }

  // DMP-LABEL: CASE_NO_PARENT_ABSENT
  // PRT-LABEL: case CASE_NO_PARENT_ABSENT
  case CASE_NO_PARENT_ABSENT:
  {
    int s, h, d;
    PRINT_VAR_INFO(d);
    PRINT_VAR_INFO(d);
    setHostInt(s, 10);
    setHostInt(h, 20);
    setHostInt(d, 30);

    // EXE-OUT-CASE_NO_PARENT_ABSENT-PASS-NEXT: updated
    updateNotNested(&s, &h, &d);

    // EXE-OUT-CASE_NO_PARENT_ABSENT-PASS-NEXT: host s=10{{$}}
    // EXE-OUT-CASE_NO_PARENT_ABSENT-PASS-NEXT: host h=20{{$}}
    // EXE-OUT-CASE_NO_PARENT_ABSENT-PASS-NEXT: host d=30{{$}}
    printHostInt(s);
    printHostInt(h);
    printHostInt(d);
    break;
  }

  //--------------------------------------------------
  // Check acc update nested statically within acc data.
  //
  // Check that dumping and printing integrate correctly with the outer
  // directive.
  //
  // Check with scalar type.
  //--------------------------------------------------

  // DMP-LABEL: CASE_SCALAR_PRESENT
  // PRT-LABEL: case CASE_SCALAR_PRESENT
  case CASE_SCALAR_PRESENT:
  // PRT-NEXT: {
  {
    // PRT-NEXT: int s, h, d;
    int s, h, d;
    // PRT-NEXT: {{PRINT_VAR_INFO|fprintf}}
    // PRT-NEXT: {{PRINT_VAR_INFO|fprintf}}
    PRINT_VAR_INFO(s);
    PRINT_VAR_INFO(s);

    //      DMP: ACCDataDirective
    // DMP-NEXT:   ACCCreateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 's' 'int'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'h' 'int'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'd' 'int'
    // DMP-NEXT:   impl: OMPTargetDataDirective
    // DMP-NEXT:     OMPMapClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 's' 'int'
    // DMP-NEXT:       DeclRefExpr {{.*}} 'h' 'int'
    // DMP-NEXT:       DeclRefExpr {{.*}} 'd' 'int'
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc data create(s,h,d){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target data map(hold,alloc: s,h,d){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp target data map(hold,alloc: s,h,d){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc data create(s,h,d){{$}}
    #pragma acc data create(s,h,d)
    // PRT-NEXT: {
    {
      // PRT-NEXT: setHostInt
      // PRT-NEXT: setHostInt
      // PRT-NEXT: setHostInt
      // PRT-NEXT: setDeviceInt
      // PRT-NEXT: setDeviceInt
      // PRT-NEXT: setDeviceInt
      setHostInt(s, 10);
      setHostInt(h, 20);
      setHostInt(d, 30);
      setDeviceInt(s, 11);
      setDeviceInt(h, 21);
      setDeviceInt(d, 31);

      //                 DMP: ACCUpdateDirective
      //            DMP-NEXT:   ACCSelfClause
      //            DMP-NEXT:     DeclRefExpr {{.*}} 's' 'int'
      //            DMP-NEXT:   ACCSelfClause
      //            DMP-NEXT:     DeclRefExpr {{.*}} 'h' 'int'
      //            DMP-NEXT:   ACCDeviceClause
      //            DMP-NEXT:     DeclRefExpr {{.*}} 'd' 'int'
      // DMP-IF_PRESENT-NEXT:   ACCIfPresentClause
      //            DMP-NEXT:   impl: OMPTargetUpdateDirective
      //            DMP-NEXT:     OMPFromClause
      //            DMP-NEXT:       DeclRefExpr {{.*}} 's' 'int'
      //            DMP-NEXT:     OMPFromClause
      //            DMP-NEXT:       DeclRefExpr {{.*}} 'h' 'int'
      //            DMP-NEXT:     OMPToClause
      //            DMP-NEXT:       DeclRefExpr {{.*}} 'd' 'int'
      //             DMP-NOT: OMP
      //
      //  PRT-A-NEXT: {{^ *}}#pragma acc update self(s) host(h) device(d){{( IF_PRESENT| if_present)?$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp target update from([[PRESENT]]s) from([[PRESENT]]h) to([[PRESENT]]d){{$}}
      //  PRT-O-NEXT: {{^ *}}#pragma omp target update from([[PRESENT]]s) from([[PRESENT]]h) to([[PRESENT]]d){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc update self(s) host(h) device(d){{( IF_PRESENT| if_present)?$}}
      #pragma acc update self(s) host(h) device(d) IF_PRESENT

      // PRT-NEXT: printHostInt
      // PRT-NEXT: printHostInt
      // PRT-NEXT: printHostInt
      // PRT-NEXT: printDeviceInt
      // PRT-NEXT: printDeviceInt
      // PRT-NEXT: printDeviceInt
      //      EXE-OUT-CASE_SCALAR_PRESENT-NEXT:   host s=11{{$}}
      //      EXE-OUT-CASE_SCALAR_PRESENT-NEXT:   host h=21{{$}}
      //  EXE-OUT-CASE_SCALAR_PRESENT-DEV-NEXT:   host d=30{{$}}
      // EXE-OUT-CASE_SCALAR_PRESENT-HOST-NEXT:   host d=31{{$}}
      //      EXE-OUT-CASE_SCALAR_PRESENT-NEXT: device s=11{{$}}
      //      EXE-OUT-CASE_SCALAR_PRESENT-NEXT: device h=21{{$}}
      //  EXE-OUT-CASE_SCALAR_PRESENT-DEV-NEXT: device d=30{{$}}
      // EXE-OUT-CASE_SCALAR_PRESENT-HOST-NEXT: device d=31{{$}}
      printHostInt(s);
      printHostInt(h);
      printHostInt(d);
      printDeviceInt(s);
      printDeviceInt(h);
      printDeviceInt(d);
    } // PRT-NEXT: }
    // PRT-NEXT: break;
    break;
  } // PRT-NEXT: }

  // DMP-LABEL: CASE_SCALAR_ABSENT
  // PRT-LABEL: case CASE_SCALAR_ABSENT
  case CASE_SCALAR_ABSENT:
  {
    int s, h, d;
    PRINT_VAR_INFO(s);
    PRINT_VAR_INFO(s);

    setHostInt(s, 10);
    setHostInt(h, 20);
    setHostInt(d, 30);

    // We need multiple directives here so we can control which clause produces
    // the runtime error.  We vary which clause produces the runtime error
    // across the various CASE_* that produce it.
    #pragma acc update self(s) IF_PRESENT
    #pragma acc update host(h) device(d) IF_PRESENT

    // EXE-OUT-CASE_SCALAR_ABSENT-PASS-NEXT: host s=10{{$}}
    // EXE-OUT-CASE_SCALAR_ABSENT-PASS-NEXT: host h=20{{$}}
    // EXE-OUT-CASE_SCALAR_ABSENT-PASS-NEXT: host d=30{{$}}
    printHostInt(s);
    printHostInt(h);
    printHostInt(d);
    break;
  }

  //--------------------------------------------------
  // Check struct.
  //--------------------------------------------------

  // DMP-LABEL: CASE_STRUCT_PRESENT
  // PRT-LABEL: case CASE_STRUCT_PRESENT
  case CASE_STRUCT_PRESENT:
  {
    struct S {
      int i;
      int j;
    } s, h, d;
    PRINT_VAR_INFO(s);
    PRINT_VAR_INFO(s);

    #pragma acc data create(s, h, d)
    {
      setHostInt(s.i, 10);
      setHostInt(s.j, 20);
      setHostInt(h.i, 30);
      setHostInt(h.j, 40);
      setHostInt(d.i, 50);
      setHostInt(d.j, 60);
      setDeviceInt(s.i, 11);
      setDeviceInt(s.j, 21);
      setDeviceInt(h.i, 31);
      setDeviceInt(h.j, 41);
      setDeviceInt(d.i, 51);
      setDeviceInt(d.j, 61);

      //                 DMP: ACCUpdateDirective
      //            DMP-NEXT:   ACCSelfClause
      //            DMP-NEXT:     DeclRefExpr {{.*}} 's' 'struct S'
      //            DMP-NEXT:   ACCSelfClause
      //            DMP-NEXT:     DeclRefExpr {{.*}} 'h' 'struct S'
      //            DMP-NEXT:   ACCDeviceClause
      //            DMP-NEXT:     DeclRefExpr {{.*}} 'd' 'struct S'
      // DMP-IF_PRESENT-NEXT:   ACCIfPresentClause
      //            DMP-NEXT:   impl: OMPTargetUpdateDirective
      //            DMP-NEXT:     OMPFromClause
      //            DMP-NEXT:       DeclRefExpr {{.*}} 's' 'struct S'
      //            DMP-NEXT:     OMPFromClause
      //            DMP-NEXT:       DeclRefExpr {{.*}} 'h' 'struct S'
      //            DMP-NEXT:     OMPToClause
      //            DMP-NEXT:       DeclRefExpr {{.*}} 'd' 'struct S'
      //             DMP-NOT: OMP
      //
      //       PRT-A: {{^ *}}#pragma acc update self(s) host(h) device(d){{( IF_PRESENT| if_present)?$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp target update from([[PRESENT]]s) from([[PRESENT]]h) to([[PRESENT]]d){{$}}
      //       PRT-O: {{^ *}}#pragma omp target update from([[PRESENT]]s) from([[PRESENT]]h) to([[PRESENT]]d){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc update self(s) host(h) device(d){{( IF_PRESENT| if_present)?$}}
      #pragma acc update self(s) host(h) device(d) IF_PRESENT

      //      EXE-OUT-CASE_STRUCT_PRESENT-NEXT:   host s.i=11{{$}}
      //      EXE-OUT-CASE_STRUCT_PRESENT-NEXT:   host s.j=21{{$}}
      //      EXE-OUT-CASE_STRUCT_PRESENT-NEXT:   host h.i=31{{$}}
      //      EXE-OUT-CASE_STRUCT_PRESENT-NEXT:   host h.j=41{{$}}
      //  EXE-OUT-CASE_STRUCT_PRESENT-DEV-NEXT:   host d.i=50{{$}}
      //  EXE-OUT-CASE_STRUCT_PRESENT-DEV-NEXT:   host d.j=60{{$}}
      // EXE-OUT-CASE_STRUCT_PRESENT-HOST-NEXT:   host d.i=51{{$}}
      // EXE-OUT-CASE_STRUCT_PRESENT-HOST-NEXT:   host d.j=61{{$}}
      //      EXE-OUT-CASE_STRUCT_PRESENT-NEXT: device s.i=11{{$}}
      //      EXE-OUT-CASE_STRUCT_PRESENT-NEXT: device s.j=21{{$}}
      //      EXE-OUT-CASE_STRUCT_PRESENT-NEXT: device h.i=31{{$}}
      //      EXE-OUT-CASE_STRUCT_PRESENT-NEXT: device h.j=41{{$}}
      //  EXE-OUT-CASE_STRUCT_PRESENT-DEV-NEXT: device d.i=50{{$}}
      //  EXE-OUT-CASE_STRUCT_PRESENT-DEV-NEXT: device d.j=60{{$}}
      // EXE-OUT-CASE_STRUCT_PRESENT-HOST-NEXT: device d.i=51{{$}}
      // EXE-OUT-CASE_STRUCT_PRESENT-HOST-NEXT: device d.j=61{{$}}
      printHostInt(s.i);
      printHostInt(s.j);
      printHostInt(h.i);
      printHostInt(h.j);
      printHostInt(d.i);
      printHostInt(d.j);
      printDeviceInt(s.i);
      printDeviceInt(s.j);
      printDeviceInt(h.i);
      printDeviceInt(h.j);
      printDeviceInt(d.i);
      printDeviceInt(d.j);
    }
    break;
  }

  // DMP-LABEL: CASE_STRUCT_ABSENT
  // PRT-LABEL: case CASE_STRUCT_ABSENT
  case CASE_STRUCT_ABSENT:
  {
    struct S {
      int i;
      int j;
    } s, h, d;
    PRINT_VAR_INFO(h);
    PRINT_VAR_INFO(h);

    setHostInt(s.i, 10);
    setHostInt(s.j, 20);
    setHostInt(h.i, 30);
    setHostInt(h.j, 40);
    setHostInt(d.i, 50);
    setHostInt(d.j, 60);

    // We need multiple directives here so we can control which clause produces
    // the runtime error.  We vary which clause produces the runtime error
    // across the various CASE_* that produce it.
    #pragma acc update host(h) IF_PRESENT
    #pragma acc update device(d) self(s) IF_PRESENT

    // EXE-OUT-CASE_STRUCT_ABSENT-PASS-NEXT: host s.i=10{{$}}
    // EXE-OUT-CASE_STRUCT_ABSENT-PASS-NEXT: host s.j=20{{$}}
    // EXE-OUT-CASE_STRUCT_ABSENT-PASS-NEXT: host h.i=30{{$}}
    // EXE-OUT-CASE_STRUCT_ABSENT-PASS-NEXT: host h.j=40{{$}}
    // EXE-OUT-CASE_STRUCT_ABSENT-PASS-NEXT: host d.i=50{{$}}
    // EXE-OUT-CASE_STRUCT_ABSENT-PASS-NEXT: host d.j=60{{$}}
    printHostInt(s.i);
    printHostInt(s.j);
    printHostInt(h.i);
    printHostInt(h.j);
    printHostInt(d.i);
    printHostInt(d.j);
    break;
  }

  //--------------------------------------------------
  // Check array.
  //--------------------------------------------------

  // DMP-LABEL: CASE_ARRAY_PRESENT
  // PRT-LABEL: case CASE_ARRAY_PRESENT
  case CASE_ARRAY_PRESENT:
  {
    int s[2];
    int h[2];
    int d[2];
    PRINT_VAR_INFO(s);
    PRINT_VAR_INFO(s);

    #pragma acc data create(s, h, d)
    {
      setHostInt(s[0], 10);
      setHostInt(s[1], 20);
      setHostInt(h[0], 30);
      setHostInt(h[1], 40);
      setHostInt(d[0], 50);
      setHostInt(d[1], 60);
      setDeviceInt(s[0], 11);
      setDeviceInt(s[1], 21);
      setDeviceInt(h[0], 31);
      setDeviceInt(h[1], 41);
      setDeviceInt(d[0], 51);
      setDeviceInt(d[1], 61);

      //                 DMP: ACCUpdateDirective
      //            DMP-NEXT:   ACCSelfClause
      //            DMP-NEXT:     DeclRefExpr {{.*}} 's' 'int [2]'
      //            DMP-NEXT:   ACCSelfClause
      //            DMP-NEXT:     DeclRefExpr {{.*}} 'h' 'int [2]'
      //            DMP-NEXT:   ACCDeviceClause
      //            DMP-NEXT:     DeclRefExpr {{.*}} 'd' 'int [2]'
      // DMP-IF_PRESENT-NEXT:   ACCIfPresentClause
      //            DMP-NEXT:   impl: OMPTargetUpdateDirective
      //            DMP-NEXT:     OMPFromClause
      //            DMP-NEXT:       DeclRefExpr {{.*}} 's' 'int [2]'
      //            DMP-NEXT:     OMPFromClause
      //            DMP-NEXT:       DeclRefExpr {{.*}} 'h' 'int [2]'
      //            DMP-NEXT:     OMPToClause
      //            DMP-NEXT:       DeclRefExpr {{.*}} 'd' 'int [2]'
      //             DMP-NOT: OMP
      //
      //       PRT-A: {{^ *}}#pragma acc update self(s) host(h) device(d){{( IF_PRESENT| if_present)?$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp target update from([[PRESENT]]s) from([[PRESENT]]h) to([[PRESENT]]d){{$}}
      //       PRT-O: {{^ *}}#pragma omp target update from([[PRESENT]]s) from([[PRESENT]]h) to([[PRESENT]]d){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc update self(s) host(h) device(d){{( IF_PRESENT| if_present)?$}}
      #pragma acc update self(s) host(h) device(d) IF_PRESENT

      //      EXE-OUT-CASE_ARRAY_PRESENT-NEXT:   host s[0]=11{{$}}
      //      EXE-OUT-CASE_ARRAY_PRESENT-NEXT:   host s[1]=21{{$}}
      //      EXE-OUT-CASE_ARRAY_PRESENT-NEXT:   host h[0]=31{{$}}
      //      EXE-OUT-CASE_ARRAY_PRESENT-NEXT:   host h[1]=41{{$}}
      //  EXE-OUT-CASE_ARRAY_PRESENT-DEV-NEXT:   host d[0]=50{{$}}
      //  EXE-OUT-CASE_ARRAY_PRESENT-DEV-NEXT:   host d[1]=60{{$}}
      // EXE-OUT-CASE_ARRAY_PRESENT-HOST-NEXT:   host d[0]=51{{$}}
      // EXE-OUT-CASE_ARRAY_PRESENT-HOST-NEXT:   host d[1]=61{{$}}
      //      EXE-OUT-CASE_ARRAY_PRESENT-NEXT: device s[0]=11{{$}}
      //      EXE-OUT-CASE_ARRAY_PRESENT-NEXT: device s[1]=21{{$}}
      //      EXE-OUT-CASE_ARRAY_PRESENT-NEXT: device h[0]=31{{$}}
      //      EXE-OUT-CASE_ARRAY_PRESENT-NEXT: device h[1]=41{{$}}
      //  EXE-OUT-CASE_ARRAY_PRESENT-DEV-NEXT: device d[0]=50{{$}}
      //  EXE-OUT-CASE_ARRAY_PRESENT-DEV-NEXT: device d[1]=60{{$}}
      // EXE-OUT-CASE_ARRAY_PRESENT-HOST-NEXT: device d[0]=51{{$}}
      // EXE-OUT-CASE_ARRAY_PRESENT-HOST-NEXT: device d[1]=61{{$}}
      printHostInt(s[0]);
      printHostInt(s[1]);
      printHostInt(h[0]);
      printHostInt(h[1]);
      printHostInt(d[0]);
      printHostInt(d[1]);
      printDeviceInt(s[0]);
      printDeviceInt(s[1]);
      printDeviceInt(h[0]);
      printDeviceInt(h[1]);
      printDeviceInt(d[0]);
      printDeviceInt(d[1]);
    }
    break;
  }

  // DMP-LABEL: CASE_ARRAY_ABSENT
  // PRT-LABEL: case CASE_ARRAY_ABSENT
  case CASE_ARRAY_ABSENT:
  {
    int s[2];
    int h[2];
    int d[2];
    PRINT_VAR_INFO(d);
    PRINT_VAR_INFO(d);

    setHostInt(s[0], 10);
    setHostInt(s[1], 20);
    setHostInt(h[0], 30);
    setHostInt(h[1], 40);
    setHostInt(d[0], 50);
    setHostInt(d[1], 60);

    // We need multiple directives here so we can control which clause produces
    // the runtime error.  We vary which clause produces the runtime error
    // across the various CASE_* that produce it.
    #pragma acc update device(d) IF_PRESENT
    #pragma acc update self(s) host(h) IF_PRESENT

    // EXE-OUT-CASE_ARRAY_ABSENT-PASS-NEXT: host s[0]=10{{$}}
    // EXE-OUT-CASE_ARRAY_ABSENT-PASS-NEXT: host s[1]=20{{$}}
    // EXE-OUT-CASE_ARRAY_ABSENT-PASS-NEXT: host h[0]=30{{$}}
    // EXE-OUT-CASE_ARRAY_ABSENT-PASS-NEXT: host h[1]=40{{$}}
    // EXE-OUT-CASE_ARRAY_ABSENT-PASS-NEXT: host d[0]=50{{$}}
    // EXE-OUT-CASE_ARRAY_ABSENT-PASS-NEXT: host d[1]=60{{$}}
    printHostInt(s[0]);
    printHostInt(s[1]);
    printHostInt(h[0]);
    printHostInt(h[1]);
    printHostInt(d[0]);
    printHostInt(d[1]);
    break;
  }

  //--------------------------------------------------
  // Check subarray not encompassing full array.
  //
  // Subarrays are checked thoroughly for acc data and acc parallel in data.c
  // parallel-subarray.c.  Check just a few cases here to be sure it basically
  // works for acc update too.
  //--------------------------------------------------

  // DMP-LABEL: CASE_SUBARRAY_PRESENT
  // PRT-LABEL: case CASE_SUBARRAY_PRESENT
  case CASE_SUBARRAY_PRESENT:
  {
    int s[4];
    int h[4];
    int d[4];
    PRINT_SUBARRAY_INFO(s, 0, 4);
    PRINT_SUBARRAY_INFO(s, 1, 2);

    #pragma acc data create(s, h, d)
    {
      setHostInt(s[0], 10);
      setHostInt(s[1], 20);
      setHostInt(s[2], 30);
      setHostInt(s[3], 40);
      setHostInt(h[0], 50);
      setHostInt(h[1], 60);
      setHostInt(h[2], 70);
      setHostInt(h[3], 80);
      setHostInt(d[0], 90);
      setHostInt(d[1], 100);
      setHostInt(d[2], 110);
      setHostInt(d[3], 120);
      setDeviceInt(s[0], 11);
      setDeviceInt(s[1], 21);
      setDeviceInt(s[2], 31);
      setDeviceInt(s[3], 41);
      setDeviceInt(h[0], 51);
      setDeviceInt(h[1], 61);
      setDeviceInt(h[2], 71);
      setDeviceInt(h[3], 81);
      setDeviceInt(d[0], 91);
      setDeviceInt(d[1], 101);
      setDeviceInt(d[2], 111);
      setDeviceInt(d[3], 121);

      //                 DMP: ACCUpdateDirective
      //            DMP-NEXT:   ACCSelfClause
      //            DMP-NEXT:     OMPArraySectionExpr
      //            DMP-NEXT:       ImplicitCastExpr {{.*}} 'int *' <ArrayToPointerDecay>
      //            DMP-NEXT:         DeclRefExpr {{.*}} 's' 'int [4]'
      //            DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
      //            DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
      //            DMP-NEXT:       <<<NULL>>>
      //            DMP-NEXT:   ACCSelfClause
      //            DMP-NEXT:     OMPArraySectionExpr
      //            DMP-NEXT:       ImplicitCastExpr {{.*}} 'int *' <ArrayToPointerDecay>
      //            DMP-NEXT:         DeclRefExpr {{.*}} 'h' 'int [4]'
      //            DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
      //            DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
      //            DMP-NEXT:       <<<NULL>>>
      //            DMP-NEXT:   ACCDeviceClause
      //            DMP-NEXT:     OMPArraySectionExpr
      //            DMP-NEXT:       ImplicitCastExpr {{.*}} 'int *' <ArrayToPointerDecay>
      //            DMP-NEXT:         DeclRefExpr {{.*}} 'd' 'int [4]'
      //            DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
      //            DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
      //            DMP-NEXT:       <<<NULL>>>
      // DMP-IF_PRESENT-NEXT:   ACCIfPresentClause
      //            DMP-NEXT:   impl: OMPTargetUpdateDirective
      //            DMP-NEXT:     OMPFromClause
      //            DMP-NEXT:       OMPArraySectionExpr
      //            DMP-NEXT:         ImplicitCastExpr {{.*}} 'int *' <ArrayToPointerDecay>
      //            DMP-NEXT:           DeclRefExpr {{.*}} 's' 'int [4]'
      //            DMP-NEXT:         IntegerLiteral {{.*}} 'int' 1
      //            DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
      //            DMP-NEXT:         <<<NULL>>>
      //            DMP-NEXT:     OMPFromClause
      //            DMP-NEXT:       OMPArraySectionExpr
      //            DMP-NEXT:         ImplicitCastExpr {{.*}} 'int *' <ArrayToPointerDecay>
      //            DMP-NEXT:           DeclRefExpr {{.*}} 'h' 'int [4]'
      //            DMP-NEXT:         IntegerLiteral {{.*}} 'int' 1
      //            DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
      //            DMP-NEXT:         <<<NULL>>>
      //            DMP-NEXT:     OMPToClause
      //            DMP-NEXT:       OMPArraySectionExpr
      //            DMP-NEXT:         ImplicitCastExpr {{.*}} 'int *' <ArrayToPointerDecay>
      //            DMP-NEXT:           DeclRefExpr {{.*}} 'd' 'int [4]'
      //            DMP-NEXT:         IntegerLiteral {{.*}} 'int' 1
      //            DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
      //            DMP-NEXT:         <<<NULL>>>
      //             DMP-NOT: OMP
      //
      //       PRT-A: {{^ *}}#pragma acc update self(s[1:2]) host(h[1:2]) device(d[1:2]){{( IF_PRESENT| if_present)?$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp target update from([[PRESENT]]s[1:2]) from([[PRESENT]]h[1:2]) to([[PRESENT]]d[1:2]){{$}}
      //       PRT-O: {{^ *}}#pragma omp target update from([[PRESENT]]s[1:2]) from([[PRESENT]]h[1:2]) to([[PRESENT]]d[1:2]){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc update self(s[1:2]) host(h[1:2]) device(d[1:2]){{( IF_PRESENT| if_present)?$}}
      #pragma acc update self(s[1:2]) host(h[1:2]) device(d[1:2]) IF_PRESENT

      //  EXE-OUT-CASE_SUBARRAY_PRESENT-DEV-NEXT:   host s[0]=10{{$}}
      //  EXE-OUT-CASE_SUBARRAY_PRESENT-DEV-NEXT:   host s[1]=21{{$}}
      //  EXE-OUT-CASE_SUBARRAY_PRESENT-DEV-NEXT:   host s[2]=31{{$}}
      //  EXE-OUT-CASE_SUBARRAY_PRESENT-DEV-NEXT:   host s[3]=40{{$}}
      //  EXE-OUT-CASE_SUBARRAY_PRESENT-DEV-NEXT:   host h[0]=50{{$}}
      //  EXE-OUT-CASE_SUBARRAY_PRESENT-DEV-NEXT:   host h[1]=61{{$}}
      //  EXE-OUT-CASE_SUBARRAY_PRESENT-DEV-NEXT:   host h[2]=71{{$}}
      //  EXE-OUT-CASE_SUBARRAY_PRESENT-DEV-NEXT:   host h[3]=80{{$}}
      //  EXE-OUT-CASE_SUBARRAY_PRESENT-DEV-NEXT:   host d[0]=90{{$}}
      //  EXE-OUT-CASE_SUBARRAY_PRESENT-DEV-NEXT:   host d[1]=100{{$}}
      //  EXE-OUT-CASE_SUBARRAY_PRESENT-DEV-NEXT:   host d[2]=110{{$}}
      //  EXE-OUT-CASE_SUBARRAY_PRESENT-DEV-NEXT:   host d[3]=120{{$}}
      // EXE-OUT-CASE_SUBARRAY_PRESENT-HOST-NEXT:   host s[0]=11{{$}}
      // EXE-OUT-CASE_SUBARRAY_PRESENT-HOST-NEXT:   host s[1]=21{{$}}
      // EXE-OUT-CASE_SUBARRAY_PRESENT-HOST-NEXT:   host s[2]=31{{$}}
      // EXE-OUT-CASE_SUBARRAY_PRESENT-HOST-NEXT:   host s[3]=41{{$}}
      // EXE-OUT-CASE_SUBARRAY_PRESENT-HOST-NEXT:   host h[0]=51{{$}}
      // EXE-OUT-CASE_SUBARRAY_PRESENT-HOST-NEXT:   host h[1]=61{{$}}
      // EXE-OUT-CASE_SUBARRAY_PRESENT-HOST-NEXT:   host h[2]=71{{$}}
      // EXE-OUT-CASE_SUBARRAY_PRESENT-HOST-NEXT:   host h[3]=81{{$}}
      // EXE-OUT-CASE_SUBARRAY_PRESENT-HOST-NEXT:   host d[0]=91{{$}}
      // EXE-OUT-CASE_SUBARRAY_PRESENT-HOST-NEXT:   host d[1]=101{{$}}
      // EXE-OUT-CASE_SUBARRAY_PRESENT-HOST-NEXT:   host d[2]=111{{$}}
      // EXE-OUT-CASE_SUBARRAY_PRESENT-HOST-NEXT:   host d[3]=121{{$}}
      //  EXE-OUT-CASE_SUBARRAY_PRESENT-DEV-NEXT: device s[0]=11{{$}}
      //  EXE-OUT-CASE_SUBARRAY_PRESENT-DEV-NEXT: device s[1]=21{{$}}
      //  EXE-OUT-CASE_SUBARRAY_PRESENT-DEV-NEXT: device s[2]=31{{$}}
      //  EXE-OUT-CASE_SUBARRAY_PRESENT-DEV-NEXT: device s[3]=41{{$}}
      //  EXE-OUT-CASE_SUBARRAY_PRESENT-DEV-NEXT: device h[0]=51{{$}}
      //  EXE-OUT-CASE_SUBARRAY_PRESENT-DEV-NEXT: device h[1]=61{{$}}
      //  EXE-OUT-CASE_SUBARRAY_PRESENT-DEV-NEXT: device h[2]=71{{$}}
      //  EXE-OUT-CASE_SUBARRAY_PRESENT-DEV-NEXT: device h[3]=81{{$}}
      //  EXE-OUT-CASE_SUBARRAY_PRESENT-DEV-NEXT: device d[0]=91{{$}}
      //  EXE-OUT-CASE_SUBARRAY_PRESENT-DEV-NEXT: device d[1]=100{{$}}
      //  EXE-OUT-CASE_SUBARRAY_PRESENT-DEV-NEXT: device d[2]=110{{$}}
      //  EXE-OUT-CASE_SUBARRAY_PRESENT-DEV-NEXT: device d[3]=121{{$}}
      // EXE-OUT-CASE_SUBARRAY_PRESENT-HOST-NEXT: device s[0]=11{{$}}
      // EXE-OUT-CASE_SUBARRAY_PRESENT-HOST-NEXT: device s[1]=21{{$}}
      // EXE-OUT-CASE_SUBARRAY_PRESENT-HOST-NEXT: device s[2]=31{{$}}
      // EXE-OUT-CASE_SUBARRAY_PRESENT-HOST-NEXT: device s[3]=41{{$}}
      // EXE-OUT-CASE_SUBARRAY_PRESENT-HOST-NEXT: device h[0]=51{{$}}
      // EXE-OUT-CASE_SUBARRAY_PRESENT-HOST-NEXT: device h[1]=61{{$}}
      // EXE-OUT-CASE_SUBARRAY_PRESENT-HOST-NEXT: device h[2]=71{{$}}
      // EXE-OUT-CASE_SUBARRAY_PRESENT-HOST-NEXT: device h[3]=81{{$}}
      // EXE-OUT-CASE_SUBARRAY_PRESENT-HOST-NEXT: device d[0]=91{{$}}
      // EXE-OUT-CASE_SUBARRAY_PRESENT-HOST-NEXT: device d[1]=101{{$}}
      // EXE-OUT-CASE_SUBARRAY_PRESENT-HOST-NEXT: device d[2]=111{{$}}
      // EXE-OUT-CASE_SUBARRAY_PRESENT-HOST-NEXT: device d[3]=121{{$}}
      printHostInt(s[0]);
      printHostInt(s[1]);
      printHostInt(s[2]);
      printHostInt(s[3]);
      printHostInt(h[0]);
      printHostInt(h[1]);
      printHostInt(h[2]);
      printHostInt(h[3]);
      printHostInt(d[0]);
      printHostInt(d[1]);
      printHostInt(d[2]);
      printHostInt(d[3]);
      printDeviceInt(s[0]);
      printDeviceInt(s[1]);
      printDeviceInt(s[2]);
      printDeviceInt(s[3]);
      printDeviceInt(h[0]);
      printDeviceInt(h[1]);
      printDeviceInt(h[2]);
      printDeviceInt(h[3]);
      printDeviceInt(d[0]);
      printDeviceInt(d[1]);
      printDeviceInt(d[2]);
      printDeviceInt(d[3]);
    }
    break;
  }

  // DMP-LABEL: CASE_SUBARRAY_PRESENT2
  // PRT-LABEL: case CASE_SUBARRAY_PRESENT2
  case CASE_SUBARRAY_PRESENT2:
  {
    int s[4];
    int h[4];
    int d[4];
    int start = 1, length = 2;
    PRINT_SUBARRAY_INFO(s, 0, 4);
    PRINT_SUBARRAY_INFO(s, start, length);

    #pragma acc data create(s, h, d)
    {
      setHostInt(s[0], 10);
      setHostInt(s[1], 20);
      setHostInt(s[2], 30);
      setHostInt(s[3], 40);
      setHostInt(h[0], 50);
      setHostInt(h[1], 60);
      setHostInt(h[2], 70);
      setHostInt(h[3], 80);
      setHostInt(d[0], 90);
      setHostInt(d[1], 100);
      setHostInt(d[2], 110);
      setHostInt(d[3], 120);
      setDeviceInt(s[0], 11);
      setDeviceInt(s[1], 21);
      setDeviceInt(s[2], 31);
      setDeviceInt(s[3], 41);
      setDeviceInt(h[0], 51);
      setDeviceInt(h[1], 61);
      setDeviceInt(h[2], 71);
      setDeviceInt(h[3], 81);
      setDeviceInt(d[0], 91);
      setDeviceInt(d[1], 101);
      setDeviceInt(d[2], 111);
      setDeviceInt(d[3], 121);

      //                 DMP: ACCUpdateDirective
      //            DMP-NEXT:   ACCSelfClause
      //            DMP-NEXT:     OMPArraySectionExpr
      //            DMP-NEXT:       ImplicitCastExpr {{.*}} 'int *' <ArrayToPointerDecay>
      //            DMP-NEXT:         DeclRefExpr {{.*}} 's' 'int [4]'
      //            DMP-NEXT:       ImplicitCastExpr {{.*}} 'int' <LValueToRValue>
      //            DMP-NEXT:       `-DeclRefExpr {{.*}} 'start' 'int'
      //            DMP-NEXT:       ImplicitCastExpr {{.*}} 'int' <LValueToRValue>
      //            DMP-NEXT:       `-DeclRefExpr {{.*}} 'length' 'int'
      //            DMP-NEXT:       <<<NULL>>>
      //            DMP-NEXT:   ACCSelfClause
      //            DMP-NEXT:     OMPArraySectionExpr
      //            DMP-NEXT:       ImplicitCastExpr {{.*}} 'int *' <ArrayToPointerDecay>
      //            DMP-NEXT:         DeclRefExpr {{.*}} 'h' 'int [4]'
      //            DMP-NEXT:       ImplicitCastExpr {{.*}} 'int' <LValueToRValue>
      //            DMP-NEXT:       `-DeclRefExpr {{.*}} 'start' 'int'
      //            DMP-NEXT:       ImplicitCastExpr {{.*}} 'int' <LValueToRValue>
      //            DMP-NEXT:       `-DeclRefExpr {{.*}} 'length' 'int'
      //            DMP-NEXT:       <<<NULL>>>
      //            DMP-NEXT:   ACCDeviceClause
      //            DMP-NEXT:     OMPArraySectionExpr
      //            DMP-NEXT:       ImplicitCastExpr {{.*}} 'int *' <ArrayToPointerDecay>
      //            DMP-NEXT:         DeclRefExpr {{.*}} 'd' 'int [4]'
      //            DMP-NEXT:       ImplicitCastExpr {{.*}} 'int' <LValueToRValue>
      //            DMP-NEXT:       `-DeclRefExpr {{.*}} 'start' 'int'
      //            DMP-NEXT:       ImplicitCastExpr {{.*}} 'int' <LValueToRValue>
      //            DMP-NEXT:       `-DeclRefExpr {{.*}} 'length' 'int'
      //            DMP-NEXT:       <<<NULL>>>
      // DMP-IF_PRESENT-NEXT:   ACCIfPresentClause
      //            DMP-NEXT:   impl: OMPTargetUpdateDirective
      //            DMP-NEXT:     OMPFromClause
      //            DMP-NEXT:       OMPArraySectionExpr
      //            DMP-NEXT:         ImplicitCastExpr {{.*}} 'int *' <ArrayToPointerDecay>
      //            DMP-NEXT:           DeclRefExpr {{.*}} 's' 'int [4]'
      //            DMP-NEXT:         ImplicitCastExpr {{.*}} 'int' <LValueToRValue>
      //            DMP-NEXT:         `-DeclRefExpr {{.*}} 'start' 'int'
      //            DMP-NEXT:         ImplicitCastExpr {{.*}} 'int' <LValueToRValue>
      //            DMP-NEXT:         `-DeclRefExpr {{.*}} 'length' 'int'
      //            DMP-NEXT:         <<<NULL>>>
      //            DMP-NEXT:     OMPFromClause
      //            DMP-NEXT:       OMPArraySectionExpr
      //            DMP-NEXT:         ImplicitCastExpr {{.*}} 'int *' <ArrayToPointerDecay>
      //            DMP-NEXT:           DeclRefExpr {{.*}} 'h' 'int [4]'
      //            DMP-NEXT:         ImplicitCastExpr {{.*}} 'int' <LValueToRValue>
      //            DMP-NEXT:         `-DeclRefExpr {{.*}} 'start' 'int'
      //            DMP-NEXT:         ImplicitCastExpr {{.*}} 'int' <LValueToRValue>
      //            DMP-NEXT:         `-DeclRefExpr {{.*}} 'length' 'int'
      //            DMP-NEXT:         <<<NULL>>>
      //            DMP-NEXT:     OMPToClause
      //            DMP-NEXT:       OMPArraySectionExpr
      //            DMP-NEXT:         ImplicitCastExpr {{.*}} 'int *' <ArrayToPointerDecay>
      //            DMP-NEXT:           DeclRefExpr {{.*}} 'd' 'int [4]'
      //            DMP-NEXT:         ImplicitCastExpr {{.*}} 'int' <LValueToRValue>
      //            DMP-NEXT:         `-DeclRefExpr {{.*}} 'start' 'int'
      //            DMP-NEXT:         ImplicitCastExpr {{.*}} 'int' <LValueToRValue>
      //            DMP-NEXT:         `-DeclRefExpr {{.*}} 'length' 'int'
      //            DMP-NEXT:         <<<NULL>>>
      //             DMP-NOT: OMP
      //
      //       PRT-A: {{^ *}}#pragma acc update self(s[start:length]) host(h[start:length]) device(d[start:length]){{( IF_PRESENT| if_present)?$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp target update from([[PRESENT]]s[start:length]) from([[PRESENT]]h[start:length]) to([[PRESENT]]d[start:length]){{$}}
      //       PRT-O: {{^ *}}#pragma omp target update from([[PRESENT]]s[start:length]) from([[PRESENT]]h[start:length]) to([[PRESENT]]d[start:length]){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc update self(s[start:length]) host(h[start:length]) device(d[start:length]){{( IF_PRESENT| if_present)?$}}
      #pragma acc update self(s[start:length]) host(h[start:length]) device(d[start:length]) IF_PRESENT

      //  EXE-OUT-CASE_SUBARRAY_PRESENT2-DEV-NEXT:   host s[0]=10{{$}}
      //  EXE-OUT-CASE_SUBARRAY_PRESENT2-DEV-NEXT:   host s[1]=21{{$}}
      //  EXE-OUT-CASE_SUBARRAY_PRESENT2-DEV-NEXT:   host s[2]=31{{$}}
      //  EXE-OUT-CASE_SUBARRAY_PRESENT2-DEV-NEXT:   host s[3]=40{{$}}
      //  EXE-OUT-CASE_SUBARRAY_PRESENT2-DEV-NEXT:   host h[0]=50{{$}}
      //  EXE-OUT-CASE_SUBARRAY_PRESENT2-DEV-NEXT:   host h[1]=61{{$}}
      //  EXE-OUT-CASE_SUBARRAY_PRESENT2-DEV-NEXT:   host h[2]=71{{$}}
      //  EXE-OUT-CASE_SUBARRAY_PRESENT2-DEV-NEXT:   host h[3]=80{{$}}
      //  EXE-OUT-CASE_SUBARRAY_PRESENT2-DEV-NEXT:   host d[0]=90{{$}}
      //  EXE-OUT-CASE_SUBARRAY_PRESENT2-DEV-NEXT:   host d[1]=100{{$}}
      //  EXE-OUT-CASE_SUBARRAY_PRESENT2-DEV-NEXT:   host d[2]=110{{$}}
      //  EXE-OUT-CASE_SUBARRAY_PRESENT2-DEV-NEXT:   host d[3]=120{{$}}
      // EXE-OUT-CASE_SUBARRAY_PRESENT2-HOST-NEXT:   host s[0]=11{{$}}
      // EXE-OUT-CASE_SUBARRAY_PRESENT2-HOST-NEXT:   host s[1]=21{{$}}
      // EXE-OUT-CASE_SUBARRAY_PRESENT2-HOST-NEXT:   host s[2]=31{{$}}
      // EXE-OUT-CASE_SUBARRAY_PRESENT2-HOST-NEXT:   host s[3]=41{{$}}
      // EXE-OUT-CASE_SUBARRAY_PRESENT2-HOST-NEXT:   host h[0]=51{{$}}
      // EXE-OUT-CASE_SUBARRAY_PRESENT2-HOST-NEXT:   host h[1]=61{{$}}
      // EXE-OUT-CASE_SUBARRAY_PRESENT2-HOST-NEXT:   host h[2]=71{{$}}
      // EXE-OUT-CASE_SUBARRAY_PRESENT2-HOST-NEXT:   host h[3]=81{{$}}
      // EXE-OUT-CASE_SUBARRAY_PRESENT2-HOST-NEXT:   host d[0]=91{{$}}
      // EXE-OUT-CASE_SUBARRAY_PRESENT2-HOST-NEXT:   host d[1]=101{{$}}
      // EXE-OUT-CASE_SUBARRAY_PRESENT2-HOST-NEXT:   host d[2]=111{{$}}
      // EXE-OUT-CASE_SUBARRAY_PRESENT2-HOST-NEXT:   host d[3]=121{{$}}
      //  EXE-OUT-CASE_SUBARRAY_PRESENT2-DEV-NEXT: device s[0]=11{{$}}
      //  EXE-OUT-CASE_SUBARRAY_PRESENT2-DEV-NEXT: device s[1]=21{{$}}
      //  EXE-OUT-CASE_SUBARRAY_PRESENT2-DEV-NEXT: device s[2]=31{{$}}
      //  EXE-OUT-CASE_SUBARRAY_PRESENT2-DEV-NEXT: device s[3]=41{{$}}
      //  EXE-OUT-CASE_SUBARRAY_PRESENT2-DEV-NEXT: device h[0]=51{{$}}
      //  EXE-OUT-CASE_SUBARRAY_PRESENT2-DEV-NEXT: device h[1]=61{{$}}
      //  EXE-OUT-CASE_SUBARRAY_PRESENT2-DEV-NEXT: device h[2]=71{{$}}
      //  EXE-OUT-CASE_SUBARRAY_PRESENT2-DEV-NEXT: device h[3]=81{{$}}
      //  EXE-OUT-CASE_SUBARRAY_PRESENT2-DEV-NEXT: device d[0]=91{{$}}
      //  EXE-OUT-CASE_SUBARRAY_PRESENT2-DEV-NEXT: device d[1]=100{{$}}
      //  EXE-OUT-CASE_SUBARRAY_PRESENT2-DEV-NEXT: device d[2]=110{{$}}
      //  EXE-OUT-CASE_SUBARRAY_PRESENT2-DEV-NEXT: device d[3]=121{{$}}
      // EXE-OUT-CASE_SUBARRAY_PRESENT2-HOST-NEXT: device s[0]=11{{$}}
      // EXE-OUT-CASE_SUBARRAY_PRESENT2-HOST-NEXT: device s[1]=21{{$}}
      // EXE-OUT-CASE_SUBARRAY_PRESENT2-HOST-NEXT: device s[2]=31{{$}}
      // EXE-OUT-CASE_SUBARRAY_PRESENT2-HOST-NEXT: device s[3]=41{{$}}
      // EXE-OUT-CASE_SUBARRAY_PRESENT2-HOST-NEXT: device h[0]=51{{$}}
      // EXE-OUT-CASE_SUBARRAY_PRESENT2-HOST-NEXT: device h[1]=61{{$}}
      // EXE-OUT-CASE_SUBARRAY_PRESENT2-HOST-NEXT: device h[2]=71{{$}}
      // EXE-OUT-CASE_SUBARRAY_PRESENT2-HOST-NEXT: device h[3]=81{{$}}
      // EXE-OUT-CASE_SUBARRAY_PRESENT2-HOST-NEXT: device d[0]=91{{$}}
      // EXE-OUT-CASE_SUBARRAY_PRESENT2-HOST-NEXT: device d[1]=101{{$}}
      // EXE-OUT-CASE_SUBARRAY_PRESENT2-HOST-NEXT: device d[2]=111{{$}}
      // EXE-OUT-CASE_SUBARRAY_PRESENT2-HOST-NEXT: device d[3]=121{{$}}
      printHostInt(s[0]);
      printHostInt(s[1]);
      printHostInt(s[2]);
      printHostInt(s[3]);
      printHostInt(h[0]);
      printHostInt(h[1]);
      printHostInt(h[2]);
      printHostInt(h[3]);
      printHostInt(d[0]);
      printHostInt(d[1]);
      printHostInt(d[2]);
      printHostInt(d[3]);
      printDeviceInt(s[0]);
      printDeviceInt(s[1]);
      printDeviceInt(s[2]);
      printDeviceInt(s[3]);
      printDeviceInt(h[0]);
      printDeviceInt(h[1]);
      printDeviceInt(h[2]);
      printDeviceInt(h[3]);
      printDeviceInt(d[0]);
      printDeviceInt(d[1]);
      printDeviceInt(d[2]);
      printDeviceInt(d[3]);
    }
    break;
  }

  //--------------------------------------------------
  // Check when data is not present.
  //--------------------------------------------------

  // DMP-LABEL: CASE_SUBARRAY_DISJOINT
  // PRT-LABEL: case CASE_SUBARRAY_DISJOINT
  case CASE_SUBARRAY_DISJOINT:
  {
    int s[4];
    int h[4];
    int d[4];
    PRINT_SUBARRAY_INFO(s, 0, 2);
    PRINT_SUBARRAY_INFO(s, 2, 2);

    #pragma acc data create(s[0:2], h[0:2], d[0:2])
    {
      setHostInt(s[0], 10);
      setHostInt(s[1], 20);
      setHostInt(s[2], 30);
      setHostInt(s[3], 40);
      setHostInt(h[0], 50);
      setHostInt(h[1], 60);
      setHostInt(h[2], 70);
      setHostInt(h[3], 80);
      setHostInt(d[0], 90);
      setHostInt(d[1], 100);
      setHostInt(d[2], 110);
      setHostInt(d[3], 120);
      setDeviceInt(s[0], 11);
      setDeviceInt(s[1], 21);
      setDeviceInt(h[0], 51);
      setDeviceInt(h[1], 61);
      setDeviceInt(d[0], 91);
      setDeviceInt(d[1], 101);

      // We need multiple directives here so we can control which clause
      // produces the runtime error.  We vary which clause produces the runtime
      // error across the various CASE_* that produce it.
      #pragma acc update self(s[2:2]) IF_PRESENT
      #pragma acc update host(s[2:2]) device(h[2:2]) IF_PRESENT

      //  EXE-OUT-CASE_SUBARRAY_DISJOINT-DEV-PASS-NEXT:   host s[0]=10{{$}}
      //  EXE-OUT-CASE_SUBARRAY_DISJOINT-DEV-PASS-NEXT:   host s[1]=20{{$}}
      //  EXE-OUT-CASE_SUBARRAY_DISJOINT-DEV-PASS-NEXT:   host s[2]=30{{$}}
      //  EXE-OUT-CASE_SUBARRAY_DISJOINT-DEV-PASS-NEXT:   host s[3]=40{{$}}
      //  EXE-OUT-CASE_SUBARRAY_DISJOINT-DEV-PASS-NEXT:   host h[0]=50{{$}}
      //  EXE-OUT-CASE_SUBARRAY_DISJOINT-DEV-PASS-NEXT:   host h[1]=60{{$}}
      //  EXE-OUT-CASE_SUBARRAY_DISJOINT-DEV-PASS-NEXT:   host h[2]=70{{$}}
      //  EXE-OUT-CASE_SUBARRAY_DISJOINT-DEV-PASS-NEXT:   host h[3]=80{{$}}
      //  EXE-OUT-CASE_SUBARRAY_DISJOINT-DEV-PASS-NEXT:   host d[0]=90{{$}}
      //  EXE-OUT-CASE_SUBARRAY_DISJOINT-DEV-PASS-NEXT:   host d[1]=100{{$}}
      //  EXE-OUT-CASE_SUBARRAY_DISJOINT-DEV-PASS-NEXT:   host d[2]=110{{$}}
      //  EXE-OUT-CASE_SUBARRAY_DISJOINT-DEV-PASS-NEXT:   host d[3]=120{{$}}
      // EXE-OUT-CASE_SUBARRAY_DISJOINT-HOST-PASS-NEXT:   host s[0]=11{{$}}
      // EXE-OUT-CASE_SUBARRAY_DISJOINT-HOST-PASS-NEXT:   host s[1]=21{{$}}
      // EXE-OUT-CASE_SUBARRAY_DISJOINT-HOST-PASS-NEXT:   host s[2]=30{{$}}
      // EXE-OUT-CASE_SUBARRAY_DISJOINT-HOST-PASS-NEXT:   host s[3]=40{{$}}
      // EXE-OUT-CASE_SUBARRAY_DISJOINT-HOST-PASS-NEXT:   host h[0]=51{{$}}
      // EXE-OUT-CASE_SUBARRAY_DISJOINT-HOST-PASS-NEXT:   host h[1]=61{{$}}
      // EXE-OUT-CASE_SUBARRAY_DISJOINT-HOST-PASS-NEXT:   host h[2]=70{{$}}
      // EXE-OUT-CASE_SUBARRAY_DISJOINT-HOST-PASS-NEXT:   host h[3]=80{{$}}
      // EXE-OUT-CASE_SUBARRAY_DISJOINT-HOST-PASS-NEXT:   host d[0]=91{{$}}
      // EXE-OUT-CASE_SUBARRAY_DISJOINT-HOST-PASS-NEXT:   host d[1]=101{{$}}
      // EXE-OUT-CASE_SUBARRAY_DISJOINT-HOST-PASS-NEXT:   host d[2]=110{{$}}
      // EXE-OUT-CASE_SUBARRAY_DISJOINT-HOST-PASS-NEXT:   host d[3]=120{{$}}
      //      EXE-OUT-CASE_SUBARRAY_DISJOINT-PASS-NEXT: device s[0]=11{{$}}
      //      EXE-OUT-CASE_SUBARRAY_DISJOINT-PASS-NEXT: device s[1]=21{{$}}
      //      EXE-OUT-CASE_SUBARRAY_DISJOINT-PASS-NEXT: device h[0]=51{{$}}
      //      EXE-OUT-CASE_SUBARRAY_DISJOINT-PASS-NEXT: device h[1]=61{{$}}
      //      EXE-OUT-CASE_SUBARRAY_DISJOINT-PASS-NEXT: device d[0]=91{{$}}
      //      EXE-OUT-CASE_SUBARRAY_DISJOINT-PASS-NEXT: device d[1]=101{{$}}
      printHostInt(s[0]);
      printHostInt(s[1]);
      printHostInt(s[2]);
      printHostInt(s[3]);
      printHostInt(h[0]);
      printHostInt(h[1]);
      printHostInt(h[2]);
      printHostInt(h[3]);
      printHostInt(d[0]);
      printHostInt(d[1]);
      printHostInt(d[2]);
      printHostInt(d[3]);
      printDeviceInt(s[0]);
      printDeviceInt(s[1]);
      printDeviceInt(h[0]);
      printDeviceInt(h[1]);
      printDeviceInt(d[0]);
      printDeviceInt(d[1]);
    }
    break;
  }

  // DMP-LABEL: CASE_SUBARRAY_OVERLAP_START
  // PRT-LABEL: case CASE_SUBARRAY_OVERLAP_START
  case CASE_SUBARRAY_OVERLAP_START:
  {
    int s[4];
    int h[4];
    int d[4];
    PRINT_SUBARRAY_INFO(h, 1, 2);
    PRINT_SUBARRAY_INFO(h, 0, 3);

    #pragma acc data create(s[1:2], h[1:2], d[1:2])
    {
      setHostInt(s[0], 10);
      setHostInt(s[1], 20);
      setHostInt(s[2], 30);
      setHostInt(s[3], 40);
      setHostInt(h[0], 50);
      setHostInt(h[1], 60);
      setHostInt(h[2], 70);
      setHostInt(h[3], 80);
      setHostInt(d[0], 90);
      setHostInt(d[1], 100);
      setHostInt(d[2], 110);
      setHostInt(d[3], 120);
      setDeviceInt(s[1], 21);
      setDeviceInt(s[2], 31);
      setDeviceInt(h[1], 61);
      setDeviceInt(h[2], 71);
      setDeviceInt(d[1], 101);
      setDeviceInt(d[2], 111);

      // We need multiple directives here so we can control which clause
      // produces the runtime error.  We vary which clause produces the runtime
      // error across the various CASE_* that produce it.
      #pragma acc update host(h[0:3]) IF_PRESENT
      #pragma acc update device(d[0:3]) self(s[0:3]) IF_PRESENT

      //  EXE-OUT-CASE_SUBARRAY_OVERLAP_START-DEV-PASS-NEXT:   host s[0]=10{{$}}
      //  EXE-OUT-CASE_SUBARRAY_OVERLAP_START-DEV-PASS-NEXT:   host s[1]=20{{$}}
      //  EXE-OUT-CASE_SUBARRAY_OVERLAP_START-DEV-PASS-NEXT:   host s[2]=30{{$}}
      //  EXE-OUT-CASE_SUBARRAY_OVERLAP_START-DEV-PASS-NEXT:   host s[3]=40{{$}}
      //  EXE-OUT-CASE_SUBARRAY_OVERLAP_START-DEV-PASS-NEXT:   host h[0]=50{{$}}
      //  EXE-OUT-CASE_SUBARRAY_OVERLAP_START-DEV-PASS-NEXT:   host h[1]=60{{$}}
      //  EXE-OUT-CASE_SUBARRAY_OVERLAP_START-DEV-PASS-NEXT:   host h[2]=70{{$}}
      //  EXE-OUT-CASE_SUBARRAY_OVERLAP_START-DEV-PASS-NEXT:   host h[3]=80{{$}}
      //  EXE-OUT-CASE_SUBARRAY_OVERLAP_START-DEV-PASS-NEXT:   host d[0]=90{{$}}
      //  EXE-OUT-CASE_SUBARRAY_OVERLAP_START-DEV-PASS-NEXT:   host d[1]=100{{$}}
      //  EXE-OUT-CASE_SUBARRAY_OVERLAP_START-DEV-PASS-NEXT:   host d[2]=110{{$}}
      //  EXE-OUT-CASE_SUBARRAY_OVERLAP_START-DEV-PASS-NEXT:   host d[3]=120{{$}}
      // EXE-OUT-CASE_SUBARRAY_OVERLAP_START-HOST-PASS-NEXT:   host s[0]=10{{$}}
      // EXE-OUT-CASE_SUBARRAY_OVERLAP_START-HOST-PASS-NEXT:   host s[1]=21{{$}}
      // EXE-OUT-CASE_SUBARRAY_OVERLAP_START-HOST-PASS-NEXT:   host s[2]=31{{$}}
      // EXE-OUT-CASE_SUBARRAY_OVERLAP_START-HOST-PASS-NEXT:   host s[3]=40{{$}}
      // EXE-OUT-CASE_SUBARRAY_OVERLAP_START-HOST-PASS-NEXT:   host h[0]=50{{$}}
      // EXE-OUT-CASE_SUBARRAY_OVERLAP_START-HOST-PASS-NEXT:   host h[1]=61{{$}}
      // EXE-OUT-CASE_SUBARRAY_OVERLAP_START-HOST-PASS-NEXT:   host h[2]=71{{$}}
      // EXE-OUT-CASE_SUBARRAY_OVERLAP_START-HOST-PASS-NEXT:   host h[3]=80{{$}}
      // EXE-OUT-CASE_SUBARRAY_OVERLAP_START-HOST-PASS-NEXT:   host d[0]=90{{$}}
      // EXE-OUT-CASE_SUBARRAY_OVERLAP_START-HOST-PASS-NEXT:   host d[1]=101{{$}}
      // EXE-OUT-CASE_SUBARRAY_OVERLAP_START-HOST-PASS-NEXT:   host d[2]=111{{$}}
      // EXE-OUT-CASE_SUBARRAY_OVERLAP_START-HOST-PASS-NEXT:   host d[3]=120{{$}}
      //      EXE-OUT-CASE_SUBARRAY_OVERLAP_START-PASS-NEXT: device s[1]=21{{$}}
      //      EXE-OUT-CASE_SUBARRAY_OVERLAP_START-PASS-NEXT: device s[2]=31{{$}}
      //      EXE-OUT-CASE_SUBARRAY_OVERLAP_START-PASS-NEXT: device h[1]=61{{$}}
      //      EXE-OUT-CASE_SUBARRAY_OVERLAP_START-PASS-NEXT: device h[2]=71{{$}}
      //      EXE-OUT-CASE_SUBARRAY_OVERLAP_START-PASS-NEXT: device d[1]=101{{$}}
      //      EXE-OUT-CASE_SUBARRAY_OVERLAP_START-PASS-NEXT: device d[2]=111{{$}}
      printHostInt(s[0]);
      printHostInt(s[1]);
      printHostInt(s[2]);
      printHostInt(s[3]);
      printHostInt(h[0]);
      printHostInt(h[1]);
      printHostInt(h[2]);
      printHostInt(h[3]);
      printHostInt(d[0]);
      printHostInt(d[1]);
      printHostInt(d[2]);
      printHostInt(d[3]);
      printDeviceInt(s[1]);
      printDeviceInt(s[2]);
      printDeviceInt(h[1]);
      printDeviceInt(h[2]);
      printDeviceInt(d[1]);
      printDeviceInt(d[2]);
    }
    break;
  }

  // DMP-LABEL: CASE_SUBARRAY_OVERLAP_END
  // PRT-LABEL: case CASE_SUBARRAY_OVERLAP_END
  case CASE_SUBARRAY_OVERLAP_END:
  {
    int s[4];
    int h[4];
    int d[4];
    PRINT_SUBARRAY_INFO(d, 1, 2);
    PRINT_SUBARRAY_INFO(d, 2, 2);

    #pragma acc data create(s[1:2], h[1:2], d[1:2])
    {
      setHostInt(s[0], 10);
      setHostInt(s[1], 20);
      setHostInt(s[2], 30);
      setHostInt(s[3], 40);
      setHostInt(h[0], 50);
      setHostInt(h[1], 60);
      setHostInt(h[2], 70);
      setHostInt(h[3], 80);
      setHostInt(d[0], 90);
      setHostInt(d[1], 100);
      setHostInt(d[2], 110);
      setHostInt(d[3], 120);
      setDeviceInt(s[1], 21);
      setDeviceInt(s[2], 31);
      setDeviceInt(h[1], 61);
      setDeviceInt(h[2], 71);
      setDeviceInt(d[1], 101);
      setDeviceInt(d[2], 111);

      // We need multiple directives here so we can control which clause
      // produces the runtime error.  We vary which clause produces the runtime
      // error across the various CASE_* that produce it.
      #pragma acc update device(d[2:2]) IF_PRESENT
      #pragma acc update self(s[2:2]) host(h[2:2]) IF_PRESENT

      //  EXE-OUT-CASE_SUBARRAY_OVERLAP_END-DEV-PASS-NEXT:   host s[0]=10{{$}}
      //  EXE-OUT-CASE_SUBARRAY_OVERLAP_END-DEV-PASS-NEXT:   host s[1]=20{{$}}
      //  EXE-OUT-CASE_SUBARRAY_OVERLAP_END-DEV-PASS-NEXT:   host s[2]=30{{$}}
      //  EXE-OUT-CASE_SUBARRAY_OVERLAP_END-DEV-PASS-NEXT:   host s[3]=40{{$}}
      //  EXE-OUT-CASE_SUBARRAY_OVERLAP_END-DEV-PASS-NEXT:   host h[0]=50{{$}}
      //  EXE-OUT-CASE_SUBARRAY_OVERLAP_END-DEV-PASS-NEXT:   host h[1]=60{{$}}
      //  EXE-OUT-CASE_SUBARRAY_OVERLAP_END-DEV-PASS-NEXT:   host h[2]=70{{$}}
      //  EXE-OUT-CASE_SUBARRAY_OVERLAP_END-DEV-PASS-NEXT:   host h[3]=80{{$}}
      //  EXE-OUT-CASE_SUBARRAY_OVERLAP_END-DEV-PASS-NEXT:   host d[0]=90{{$}}
      //  EXE-OUT-CASE_SUBARRAY_OVERLAP_END-DEV-PASS-NEXT:   host d[1]=100{{$}}
      //  EXE-OUT-CASE_SUBARRAY_OVERLAP_END-DEV-PASS-NEXT:   host d[2]=110{{$}}
      //  EXE-OUT-CASE_SUBARRAY_OVERLAP_END-DEV-PASS-NEXT:   host d[3]=120{{$}}
      // EXE-OUT-CASE_SUBARRAY_OVERLAP_END-HOST-PASS-NEXT:   host s[0]=10{{$}}
      // EXE-OUT-CASE_SUBARRAY_OVERLAP_END-HOST-PASS-NEXT:   host s[1]=21{{$}}
      // EXE-OUT-CASE_SUBARRAY_OVERLAP_END-HOST-PASS-NEXT:   host s[2]=31{{$}}
      // EXE-OUT-CASE_SUBARRAY_OVERLAP_END-HOST-PASS-NEXT:   host s[3]=40{{$}}
      // EXE-OUT-CASE_SUBARRAY_OVERLAP_END-HOST-PASS-NEXT:   host h[0]=50{{$}}
      // EXE-OUT-CASE_SUBARRAY_OVERLAP_END-HOST-PASS-NEXT:   host h[1]=61{{$}}
      // EXE-OUT-CASE_SUBARRAY_OVERLAP_END-HOST-PASS-NEXT:   host h[2]=71{{$}}
      // EXE-OUT-CASE_SUBARRAY_OVERLAP_END-HOST-PASS-NEXT:   host h[3]=80{{$}}
      // EXE-OUT-CASE_SUBARRAY_OVERLAP_END-HOST-PASS-NEXT:   host d[0]=90{{$}}
      // EXE-OUT-CASE_SUBARRAY_OVERLAP_END-HOST-PASS-NEXT:   host d[1]=101{{$}}
      // EXE-OUT-CASE_SUBARRAY_OVERLAP_END-HOST-PASS-NEXT:   host d[2]=111{{$}}
      // EXE-OUT-CASE_SUBARRAY_OVERLAP_END-HOST-PASS-NEXT:   host d[3]=120{{$}}
      //      EXE-OUT-CASE_SUBARRAY_OVERLAP_END-PASS-NEXT: device s[1]=21{{$}}
      //      EXE-OUT-CASE_SUBARRAY_OVERLAP_END-PASS-NEXT: device s[2]=31{{$}}
      //      EXE-OUT-CASE_SUBARRAY_OVERLAP_END-PASS-NEXT: device h[1]=61{{$}}
      //      EXE-OUT-CASE_SUBARRAY_OVERLAP_END-PASS-NEXT: device h[2]=71{{$}}
      //      EXE-OUT-CASE_SUBARRAY_OVERLAP_END-PASS-NEXT: device d[1]=101{{$}}
      //      EXE-OUT-CASE_SUBARRAY_OVERLAP_END-PASS-NEXT: device d[2]=111{{$}}
      printHostInt(s[0]);
      printHostInt(s[1]);
      printHostInt(s[2]);
      printHostInt(s[3]);
      printHostInt(h[0]);
      printHostInt(h[1]);
      printHostInt(h[2]);
      printHostInt(h[3]);
      printHostInt(d[0]);
      printHostInt(d[1]);
      printHostInt(d[2]);
      printHostInt(d[3]);
      printDeviceInt(s[1]);
      printDeviceInt(s[2]);
      printDeviceInt(h[1]);
      printDeviceInt(h[2]);
      printDeviceInt(d[1]);
      printDeviceInt(d[2]);
    }
    break;
  }

  // DMP-LABEL: CASE_SUBARRAY_CONCAT2
  // PRT-LABEL: case CASE_SUBARRAY_CONCAT2
  case CASE_SUBARRAY_CONCAT2:
  {
    int s[4];
    int h[4];
    int d[4];
    PRINT_SUBARRAY_INFO(s, 0, 2);
    PRINT_SUBARRAY_INFO(s, 0, 4);

    #pragma acc data create(s[0:2], h[0:2], d[0:2])
    #pragma acc data create(s[2:2], h[2:2], d[2:2])
    {
      setHostInt(s[0], 10);
      setHostInt(s[1], 20);
      setHostInt(s[2], 30);
      setHostInt(s[3], 40);
      setHostInt(h[0], 50);
      setHostInt(h[1], 60);
      setHostInt(h[2], 70);
      setHostInt(h[3], 80);
      setHostInt(d[0], 90);
      setHostInt(d[1], 100);
      setHostInt(d[2], 110);
      setHostInt(d[3], 120);
      setDeviceInt(s[0], 11);
      setDeviceInt(s[1], 21);
      setDeviceInt(s[2], 31);
      setDeviceInt(s[3], 41);
      setDeviceInt(h[0], 51);
      setDeviceInt(h[1], 61);
      setDeviceInt(h[2], 71);
      setDeviceInt(h[3], 81);
      setDeviceInt(d[0], 91);
      setDeviceInt(d[1], 101);
      setDeviceInt(d[2], 111);
      setDeviceInt(d[3], 121);

      // We need multiple directives here so we can control which clause
      // produces the runtime error.  We vary which clause produces the runtime
      // error across the various CASE_* that produce it.
      #pragma acc update self(s[0:4]) IF_PRESENT
      #pragma acc update host(h[0:4]) device(d[0:4]) IF_PRESENT

      //  EXE-OUT-CASE_SUBARRAY_CONCAT2-DEV-PASS-NEXT:   host s[0]=10{{$}}
      //  EXE-OUT-CASE_SUBARRAY_CONCAT2-DEV-PASS-NEXT:   host s[1]=20{{$}}
      //  EXE-OUT-CASE_SUBARRAY_CONCAT2-DEV-PASS-NEXT:   host s[2]=30{{$}}
      //  EXE-OUT-CASE_SUBARRAY_CONCAT2-DEV-PASS-NEXT:   host s[3]=40{{$}}
      //  EXE-OUT-CASE_SUBARRAY_CONCAT2-DEV-PASS-NEXT:   host h[0]=50{{$}}
      //  EXE-OUT-CASE_SUBARRAY_CONCAT2-DEV-PASS-NEXT:   host h[1]=60{{$}}
      //  EXE-OUT-CASE_SUBARRAY_CONCAT2-DEV-PASS-NEXT:   host h[2]=70{{$}}
      //  EXE-OUT-CASE_SUBARRAY_CONCAT2-DEV-PASS-NEXT:   host h[3]=80{{$}}
      //  EXE-OUT-CASE_SUBARRAY_CONCAT2-DEV-PASS-NEXT:   host d[0]=90{{$}}
      //  EXE-OUT-CASE_SUBARRAY_CONCAT2-DEV-PASS-NEXT:   host d[1]=100{{$}}
      //  EXE-OUT-CASE_SUBARRAY_CONCAT2-DEV-PASS-NEXT:   host d[2]=110{{$}}
      //  EXE-OUT-CASE_SUBARRAY_CONCAT2-DEV-PASS-NEXT:   host d[3]=120{{$}}
      // EXE-OUT-CASE_SUBARRAY_CONCAT2-HOST-PASS-NEXT:   host s[0]=11{{$}}
      // EXE-OUT-CASE_SUBARRAY_CONCAT2-HOST-PASS-NEXT:   host s[1]=21{{$}}
      // EXE-OUT-CASE_SUBARRAY_CONCAT2-HOST-PASS-NEXT:   host s[2]=31{{$}}
      // EXE-OUT-CASE_SUBARRAY_CONCAT2-HOST-PASS-NEXT:   host s[3]=41{{$}}
      // EXE-OUT-CASE_SUBARRAY_CONCAT2-HOST-PASS-NEXT:   host h[0]=51{{$}}
      // EXE-OUT-CASE_SUBARRAY_CONCAT2-HOST-PASS-NEXT:   host h[1]=61{{$}}
      // EXE-OUT-CASE_SUBARRAY_CONCAT2-HOST-PASS-NEXT:   host h[2]=71{{$}}
      // EXE-OUT-CASE_SUBARRAY_CONCAT2-HOST-PASS-NEXT:   host h[3]=81{{$}}
      // EXE-OUT-CASE_SUBARRAY_CONCAT2-HOST-PASS-NEXT:   host d[0]=91{{$}}
      // EXE-OUT-CASE_SUBARRAY_CONCAT2-HOST-PASS-NEXT:   host d[1]=101{{$}}
      // EXE-OUT-CASE_SUBARRAY_CONCAT2-HOST-PASS-NEXT:   host d[2]=111{{$}}
      // EXE-OUT-CASE_SUBARRAY_CONCAT2-HOST-PASS-NEXT:   host d[3]=121{{$}}
      //      EXE-OUT-CASE_SUBARRAY_CONCAT2-PASS-NEXT: device s[0]=11{{$}}
      //      EXE-OUT-CASE_SUBARRAY_CONCAT2-PASS-NEXT: device s[1]=21{{$}}
      //      EXE-OUT-CASE_SUBARRAY_CONCAT2-PASS-NEXT: device s[2]=31{{$}}
      //      EXE-OUT-CASE_SUBARRAY_CONCAT2-PASS-NEXT: device s[3]=41{{$}}
      //      EXE-OUT-CASE_SUBARRAY_CONCAT2-PASS-NEXT: device h[0]=51{{$}}
      //      EXE-OUT-CASE_SUBARRAY_CONCAT2-PASS-NEXT: device h[1]=61{{$}}
      //      EXE-OUT-CASE_SUBARRAY_CONCAT2-PASS-NEXT: device h[2]=71{{$}}
      //      EXE-OUT-CASE_SUBARRAY_CONCAT2-PASS-NEXT: device h[3]=81{{$}}
      //      EXE-OUT-CASE_SUBARRAY_CONCAT2-PASS-NEXT: device d[0]=91{{$}}
      //      EXE-OUT-CASE_SUBARRAY_CONCAT2-PASS-NEXT: device d[1]=101{{$}}
      //      EXE-OUT-CASE_SUBARRAY_CONCAT2-PASS-NEXT: device d[2]=111{{$}}
      //      EXE-OUT-CASE_SUBARRAY_CONCAT2-PASS-NEXT: device d[3]=121{{$}}
      printHostInt(s[0]);
      printHostInt(s[1]);
      printHostInt(s[2]);
      printHostInt(s[3]);
      printHostInt(h[0]);
      printHostInt(h[1]);
      printHostInt(h[2]);
      printHostInt(h[3]);
      printHostInt(d[0]);
      printHostInt(d[1]);
      printHostInt(d[2]);
      printHostInt(d[3]);
      printDeviceInt(s[0]);
      printDeviceInt(s[1]);
      printDeviceInt(s[2]);
      printDeviceInt(s[3]);
      printDeviceInt(h[0]);
      printDeviceInt(h[1]);
      printDeviceInt(h[2]);
      printDeviceInt(h[3]);
      printDeviceInt(d[0]);
      printDeviceInt(d[1]);
      printDeviceInt(d[2]);
      printDeviceInt(d[3]);
    }
    break;
  }

  case CASE_END:
    fprintf(stderr, "unexpected CASE_END\n");
    break;
  }

  //          EXE-ERR-NOT: {{.}}
  //              EXE-ERR: addr=0x[[#%x,OLD_MAP_ADDR:]], size=[[#%u,OLD_MAP_SIZE:]]
  //         EXE-ERR-NEXT: addr=0x[[#%x,NEW_MAP_ADDR:]], size=[[#%u,NEW_MAP_SIZE:]]
  // EXE-ERR-notPASS-NEXT: Libomptarget message: device mapping required by 'present' motion modifier does not exist for host address 0x{{0*}}[[#NEW_MAP_ADDR]] ([[#NEW_MAP_SIZE]] bytes)
  // EXE-ERR-notPASS-NEXT: Libomptarget fatal error 1: failure of target construct while offloading is mandatory
  //                       # An abort message usually follows.
  //  EXE-ERR-notPASS-NOT: Libomptarget
  //     EXE-ERR-PASS-NOT: {{.}}

  // PRT-LABEL: return 0;
  return 0;
}

// EXE-OUT-NOT: {{.}}
