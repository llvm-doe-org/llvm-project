// Abbreviations:
//   A       = OpenACC
//   AO      = commented OpenMP is printed after OpenACC
//   O       = OpenMP
//   OA      = commented OpenACC is printed after OpenMP
//   AOX     = either AO or OA
//   AIMP    = OpenACC implicit independent
//   AIND    = OpenACC explicit independent clause
//   ASEQ    = OpenACC seq clause
//   AAUTO   = OpenACC auto clause
//   AG      = OpenACC gang clause
//   AW      = OpenACC worker clause
//   AV      = OpenACC vector clause
//   ASLC    = OpenACC shared clause for assigned loop control variable
//   APLC    = OpenACC private clause for assigned loop control variable
//   OPRG    = OpenACC pragma translated to OpenMP pragma
//   OPLC    = OPRG but any assigned loop control var becomes declared private
//             in an enclosing compound statement
//   OSEQ    = OpenACC loop seq discarded in translation to OpenMP
//   OSIMP   = OpenMP shared implicit
//   OSEXP   = OpenMP shared explicit
//   ONT1    = OpenMP num_threads(1)
//   PART    = partitioned
//   NOPART  = not partitioned
//   GREDUN  = gang-redundant
//   GPART   = gang-partitioned
//   SLC     = shared loop control variable
//   PLC     = private loop control variable
//
//   accc    = OpenACC clauses
//   accc_sp = space before OpenACC clauses or none if no clauses
//   ompdd   = OpenMP directive dump
//   ompdp   = OpenMP directive print
//   ompdk   = OpenMP directive kind (OPRG, OPLC, or OSEQ)
//   ompsk   = OpenMP shared kind (OSIMP or OSEXP)
//   dmp     = additional FileCheck prefixes for dump
//   exe     = additional FileCheck prefixes for execution
//
// RUN: %data loop-clauses {
//        implicit independent, seq, independent, auto
// RUN:   (accc=
// RUN:    accc_sp=
// RUN:    ompdd=
// RUN:    ompdp=
// RUN:    ompdk=OSEQ
// RUN:    ompsk=OSIMP
// RUN:    dmp=DMP-AIMP,DMP-ASLC
// RUN:    exe=EXE,EXE-NOPART,EXE-GREDUN,EXE-SLC)
// RUN:   (accc=seq
// RUN:    accc_sp=' '
// RUN:    ompdd=
// RUN:    ompdp=
// RUN:    ompdk=OSEQ
// RUN:    ompsk=OSIMP
// RUN:    dmp=DMP-ASEQ,DMP-ASLC
// RUN:    exe=EXE,EXE-NOPART,EXE-GREDUN,EXE-SLC)
// RUN:   (accc=independent
// RUN:    accc_sp=' '
// RUN:    ompdd=
// RUN:    ompdp=
// RUN:    ompdk=OSEQ
// RUN:    ompsk=OSIMP
// RUN:    dmp=DMP-AIND,DMP-ASLC
// RUN:    exe=EXE,EXE-NOPART,EXE-GREDUN,EXE-SLC)
// RUN:   (accc=auto
// RUN:    accc_sp=' '
// RUN:    ompdd=
// RUN:    ompdp=
// RUN:    ompdk=OSEQ
// RUN:    ompsk=OSIMP
// RUN:    dmp=DMP-AAUTO,DMP-ASLC
// RUN:    exe=EXE,EXE-NOPART,EXE-GREDUN,EXE-SLC)
//
//        gang, worker, vector with each of the above except seq
// RUN:   (accc=gang
// RUN:    accc_sp=' '
// RUN:    ompdd=OMPDistributeDirective
// RUN:    ompdp=distribute
// RUN:    ompdk=OPRG
// RUN:    ompsk=OSIMP
// RUN:    dmp=DMP-AIMP,DMP-APLC,DMP-AG
// RUN:    exe=EXE,EXE-PART,EXE-GPART,EXE-PLC)
// RUN:   (accc=worker
// RUN:    accc_sp=' '
// RUN:    ompdd=OMPParallelForDirective
// RUN:    ompdp='parallel for'
// RUN:    ompdk=OPRG
// RUN:    ompsk=OSEXP
// RUN:    dmp=DMP-AIMP,DMP-APLC,DMP-AW
// RUN:    exe=EXE,EXE-PART,EXE-GREDUN,EXE-PLC)
// RUN:   (accc=vector
// RUN:    accc_sp=' '
// RUN:    ompdd=OMPParallelForSimdDirective
// RUN:    ompdp='parallel for simd num_threads(1)'
// RUN:    ompdk=OPLC
// RUN:    ompsk=OSEXP
// RUN:    dmp=DMP-AIMP,DMP-APLC,DMP-AV,DMP-ONT1
// RUN:    exe=EXE,EXE-PART,EXE-GREDUN,EXE-PLC)
// RUN:   (accc='independent gang'
// RUN:    accc_sp=' '
// RUN:    ompdd=OMPDistributeDirective
// RUN:    ompdp='distribute'
// RUN:    ompdk=OPRG
// RUN:    ompsk=OSIMP
// RUN:    dmp=DMP-AIND,DMP-APLC,DMP-AG
// RUN:    exe=EXE,EXE-PART,EXE-GPART,EXE-PLC)
// RUN:   (accc='independent worker'
// RUN:    accc_sp=' '
// RUN:    ompdd=OMPParallelForDirective
// RUN:    ompdp='parallel for'
// RUN:    ompdk=OPRG
// RUN:    ompsk=OSEXP
// RUN:    dmp=DMP-AIND,DMP-APLC,DMP-AW
// RUN:    exe=EXE,EXE-PART,EXE-GREDUN,EXE-PLC)
// RUN:   (accc='independent vector'
// RUN:    accc_sp=' '
// RUN:    ompdd=OMPParallelForSimdDirective
// RUN:    ompdp='parallel for simd num_threads(1)'
// RUN:    ompdk=OPLC
// RUN:    ompsk=OSEXP
// RUN:    dmp=DMP-AIND,DMP-APLC,DMP-AV,DMP-ONT1
// RUN:    exe=EXE,EXE-PART,EXE-GREDUN,EXE-PLC)
// RUN:   (accc='auto gang'
// RUN:    accc_sp=' '
// RUN:    ompdd=
// RUN:    ompdp=
// RUN:    ompdk=OSEQ
// RUN:    ompsk=OSIMP
// RUN:    dmp=DMP-AAUTO,DMP-ASLC,DMP-AG
// RUN:    exe=EXE,EXE-NOPART,EXE-GREDUN,EXE-SLC)
// RUN:   (accc='auto worker'
// RUN:    accc_sp=' '
// RUN:    ompdd=
// RUN:    ompdp=
// RUN:    ompdk=OSEQ
// RUN:    ompsk=OSIMP
// RUN:    dmp=DMP-AAUTO,DMP-ASLC,DMP-AW
// RUN:    exe=EXE,EXE-NOPART,EXE-GREDUN,EXE-SLC)
// RUN:   (accc='auto vector'
// RUN:    accc_sp=' '
// RUN:    ompdd=
// RUN:    ompdp=
// RUN:    ompdk=OSEQ
// RUN:    ompsk=OSIMP
// RUN:    dmp=DMP-AAUTO,DMP-ASLC,DMP-AV
// RUN:    exe=EXE,EXE-NOPART,EXE-GREDUN,EXE-SLC)
//
//        combinations of gang, worker, vector
// RUN:   (accc='gang worker'
// RUN:    accc_sp=' '
// RUN:    ompdd=OMPDistributeParallelForDirective
// RUN:    ompdp='distribute parallel for'
// RUN:    ompdk=OPRG
// RUN:    ompsk=OSEXP
// RUN:    dmp=DMP-AIMP,DMP-APLC,DMP-AG,DMP-AW
// RUN:    exe=EXE,EXE-PART,EXE-GPART,EXE-PLC)
// RUN:   (accc='independent gang worker'
// RUN:    accc_sp=' '
// RUN:    ompdd=OMPDistributeParallelForDirective
// RUN:    ompdp='distribute parallel for'
// RUN:    ompdk=OPRG
// RUN:    ompsk=OSEXP
// RUN:    dmp=DMP-AIND,DMP-APLC,DMP-AG,DMP-AW
// RUN:    exe=EXE,EXE-PART,EXE-GPART,EXE-PLC)
// RUN:   (accc='gang vector'
// RUN:    accc_sp=' '
// RUN:    ompdd=OMPDistributeSimdDirective
// RUN:    ompdp='distribute simd'
// RUN:    ompdk=OPLC
// RUN:    ompsk=OSIMP
// RUN:    dmp=DMP-AIMP,DMP-APLC,DMP-AG,DMP-AV
// RUN:    exe=EXE,EXE-PART,EXE-GPART,EXE-PLC)
// RUN:   (accc='independent gang vector'
// RUN:    accc_sp=' '
// RUN:    ompdd=OMPDistributeSimdDirective
// RUN:    ompdp='distribute simd'
// RUN:    ompdk=OPLC
// RUN:    ompsk=OSIMP
// RUN:    dmp=DMP-AIND,DMP-APLC,DMP-AG,DMP-AV
// RUN:    exe=EXE,EXE-PART,EXE-GPART,EXE-PLC)
// RUN:   (accc='worker vector'
// RUN:    accc_sp=' '
// RUN:    ompdd=OMPParallelForSimdDirective
// RUN:    ompdp='parallel for simd'
// RUN:    ompdk=OPLC
// RUN:    ompsk=OSEXP
// RUN:    dmp=DMP-AIMP,DMP-APLC,DMP-AW,DMP-AV
// RUN:    exe=EXE,EXE-PART,EXE-GREDUN,EXE-PLC)
// RUN:   (accc='independent worker vector'
// RUN:    accc_sp=' '
// RUN:    ompdd=OMPParallelForSimdDirective
// RUN:    ompdp='parallel for simd'
// RUN:    ompdk=OPLC
// RUN:    ompsk=OSEXP
// RUN:    dmp=DMP-AIND,DMP-APLC,DMP-AW,DMP-AV
// RUN:    exe=EXE,EXE-PART,EXE-GREDUN,EXE-PLC)
// RUN:   (accc='gang worker vector'
// RUN:    accc_sp=' '
// RUN:    ompdd=OMPDistributeParallelForSimdDirective
// RUN:    ompdp='distribute parallel for simd'
// RUN:    ompdk=OPLC
// RUN:    ompsk=OSEXP
// RUN:    dmp=DMP-AIMP,DMP-APLC,DMP-AG,DMP-AW,DMP-AV
// RUN:    exe=EXE,EXE-PART,EXE-GPART,EXE-PLC)
// RUN:   (accc='independent gang worker vector'
// RUN:    accc_sp=' '
// RUN:    ompdd=OMPDistributeParallelForSimdDirective
// RUN:    ompdp='distribute parallel for simd'
// RUN:    ompdk=OPLC
// RUN:    ompsk=OSEXP
// RUN:    dmp=DMP-AIND,DMP-APLC,DMP-AG,DMP-AW,DMP-AV
// RUN:    exe=EXE,EXE-PART,EXE-GPART,EXE-PLC)
// RUN:   (accc='auto gang worker'
// RUN:    accc_sp=' '
// RUN:    ompdd=
// RUN:    ompdp=
// RUN:    ompdk=OSEQ
// RUN:    ompsk=OSIMP
// RUN:    dmp=DMP-AAUTO,DMP-ASLC,DMP-AG,DMP-AW
// RUN:    exe=EXE,EXE-NOPART,EXE-GREDUN,EXE-SLC)
// RUN:   (accc='auto gang vector'
// RUN:    accc_sp=' '
// RUN:    ompdd=
// RUN:    ompdp=
// RUN:    ompdk=OSEQ
// RUN:    ompsk=OSIMP
// RUN:    dmp=DMP-AAUTO,DMP-ASLC,DMP-AG,DMP-AV
// RUN:    exe=EXE,EXE-NOPART,EXE-GREDUN,EXE-SLC)
// RUN:   (accc='auto worker vector'
// RUN:    accc_sp=' '
// RUN:    ompdd=
// RUN:    ompdp=
// RUN:    ompdk=OSEQ
// RUN:    ompsk=OSIMP
// RUN:    dmp=DMP-AAUTO,DMP-ASLC,DMP-AW,DMP-AV
// RUN:    exe=EXE,EXE-NOPART,EXE-GREDUN,EXE-SLC)
// RUN:   (accc='auto gang worker vector'
// RUN:    accc_sp=' '
// RUN:    ompdd=
// RUN:    ompdp=
// RUN:    ompdk=OSEQ
// RUN:    ompsk=OSIMP
// RUN:    dmp=DMP-AAUTO,DMP-ASLC,DMP-AG,DMP-AW,DMP-AV
// RUN:    exe=EXE,EXE-NOPART,EXE-GREDUN,EXE-SLC)
// RUN: }

// Check ASTDumper.
//
// RUN: %for loop-clauses {
// RUN:   %clang -Xclang -verify -Xclang -ast-dump -fsyntax-only -fopenacc \
// RUN:          -DACCC=%'accc' %s \
// RUN:   | FileCheck %s \
// RUN:       -check-prefixes=DMP,DMP-%[ompdk],DMP-%[ompdk]-%[ompsk],%[dmp] \
// RUN:       -DOMPDD=%[ompdd]
// RUN: }

// Check -ast-print and -fopenacc-print.
//
// RUN: %clang -Xclang -verify -Xclang -ast-print -fsyntax-only %s \
// RUN: | FileCheck -check-prefixes=PRT %s
//
// RUN: %data prints {
// RUN:   (print='-Xclang -ast-print -fsyntax-only -fopenacc' prt=PRT,PRT-A)
// RUN:   (print=-fopenacc-print=acc     prt=PRT,PRT-A)
// RUN:   (print=-fopenacc-print=omp     prt=PRT,PRT-O,PRT-O-%[ompdk],PRT-O-%[ompdk]-%[ompsk])
// RUN:   (print=-fopenacc-print=acc-omp prt=PRT,PRT-A,PRT-AOX,PRT-AOX-%[ompdk],PRT-AOX-%[ompdk]-%[ompsk],PRT-AO,PRT-AO-%[ompdk],PRT-AO-%[ompdk]-%[ompsk])
// RUN:   (print=-fopenacc-print=omp-acc prt=PRT,PRT-O,PRT-O-%[ompdk],PRT-O-%[ompdk]-%[ompsk],PRT-AOX,PRT-AOX-%[ompdk],PRT-AOX-%[ompdk]-%[ompsk],PRT-OA,PRT-OA-%[ompdk],PRT-OA-%[ompdk]-%[ompsk])
// RUN: }
// RUN: %for loop-clauses {
// RUN:   %for prints {
// RUN:     %clang -Xclang -verify %[print] -DACCC=%'accc' %s \
// RUN:     | FileCheck -check-prefixes=%[prt] -DACCC=%'accc_sp'%'accc' \
// RUN:                 -DOMPDP=%'ompdp' %s
// RUN:   }
// RUN: }

// Check ASTWriterStmt, ASTReaderStmt, StmtPrinter, and
// ACCLoopDirective::CreateEmpty (used by ASTReaderStmt).  Some data related to
// printing (where to print comments about discarded directives) is serialized
// and deserialized, so it's worthwhile to try all OpenACC printing modes.
//
// RUN: %for loop-clauses {
// RUN:   %clang -Xclang -verify -fopenacc -emit-ast %s -DACCC=%'accc' \
// RUN:          -o %t.ast
// RUN:   %for prints {
// RUN:     %clang %[print] %t.ast 2>&1 \
// RUN:     | FileCheck -check-prefixes=%[prt] -DACCC=%'accc_sp'%'accc' \
// RUN:                 -DOMPDP=%'ompdp' %s
// RUN:   }
// RUN: }

// Can we -ast-print the OpenMP source code, compile, and run it successfully?
//
// RUN: %for loop-clauses {
// RUN:   %clang -Xclang -verify -fopenacc-print=omp -DACCC=%'accc' %s \
// RUN:          > %t-omp.c
// RUN:   echo "// expected""-no-diagnostics" >> %t-omp.c
// RUN:   %clang -Xclang -verify -fopenmp -o %t %t-omp.c
// RUN:   %t | FileCheck -check-prefixes=%[exe] %s
// RUN: }

// Check execution with normal compilation.
//
// RUN: %for loop-clauses {
// RUN:   %clang -Xclang -verify -fopenacc -DACCC=%'accc' %s -o %t
// RUN:   %t 2 2>&1 | FileCheck -check-prefixes=%[exe] %s
// RUN: }

// END.

// expected-no-diagnostics

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
  struct {
    bool declNestedLoopOld;
    bool declNestedLoopNew;
    bool declNestedLoopErr;

    bool assignLoopOld;
    bool assignLoopNew;
    bool assignLoopErr;

    bool assignNestedLoopOld;
    bool assignNestedLoopNew;
    bool assignNestedLoopErr;

    bool readInLaterLoopOld;
    bool readInLaterLoopNew;
    bool readInLaterLoopErr;

    bool writeInLaterLoopOld;
    bool writeInLaterLoopNew;
    bool writeInLaterLoopErr;

    bool readLoopOnlyOld;
    bool readLoopOnlyNew;
    bool readLoopOnlyErr;

    bool readLoopOnlyArrOld;
    bool readLoopOnlyArrNew;
    bool readLoopOnlyArrErr;
  } save;

  memset(&save, 0, sizeof(save));

  // Accessed in acc loop but not otherwise in acc parallel.
  // PRT:      int loopOnly = 88;
  // PRT-NEXT: int loopOnlyArr[1] = {88};
  int loopOnly = 88;
  int loopOnlyArr[1] = {88};

  // All loops print the same thing because there is no barrier between them
  // and so we cannot be sure of the output order when checking it.
  //
  // DMP:      ACCParallelDirective
  // DMP-NEXT:   ACCNum_gangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 3
  // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  // DMP-NEXT:     DeclRefExpr {{.*}} 'save'
  // DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnlyArr' 'int [1]'
  // DMP-NEXT:   ACCFirstprivateClause {{.*}} <implicit>
  // DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnly' 'int'
  // DMP-NEXT:   impl: OMPTargetTeamsDirective
  // DMP-NEXT:     OMPNum_teamsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 3
  // DMP-NEXT:     OMPSharedClause
  // DMP-NOT:        <implicit>
  // DMP-NEXT:       DeclRefExpr {{.*}} 'save'
  // DMP-NEXT:       DeclRefExpr {{.*}} 'loopOnlyArr' 'int [1]'
  // DMP-NEXT:     OMPFirstprivateClause
  // DMP-NOT:        <implicit>
  // DMP-NEXT:       DeclRefExpr {{.*}} 'loopOnly' 'int'
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(3){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(3) shared(save,loopOnlyArr) firstprivate(loopOnly){{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(3) shared(save,loopOnlyArr) firstprivate(loopOnly){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(3){{$}}
  #pragma acc parallel num_gangs(3)
  // DMP: CompoundStmt
  // PRT-NEXT: {
  {

    // PRT-NEXT: int j = 0;
    // PRT-NEXT: int k = 0;
    int j = 0;
    int k = 0;

    // Exercise declared loop control variable, i.
    //
    // Despite being a control variable on an inner loop, j shouldn't be
    // handled as the control variable for the enclosing acc loop and so
    // shouldn't be predetermined private.
    //
    // DMP:               ACCLoopDirective
    // DMP-ASEQ-NEXT:       ACCSeqClause
    // DMP-AIND-NEXT:       ACCIndependentClause
    // DMP-AAUTO-NEXT:      ACCAutoClause
    // DMP-ASEQ-NOT:        <implicit>
    // DMP-AIND-NOT:        <implicit>
    // DMP-AAUTO-NOT:       <implicit>
    // DMP-AG-NEXT:         ACCGangClause
    // DMP-AW-NEXT:         ACCWorkerClause
    // DMP-AV-NEXT:         ACCVectorClause
    // DMP-AIMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:            ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:              DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:              DeclRefExpr {{.*}} 'loopOnly' 'int'
    // DMP-NEXT:              DeclRefExpr {{.*}} 'save'
    // DMP-NEXT:              DeclRefExpr {{.*}} 'loopOnlyArr' 'int [1]'
    // DMP-OPRG-NEXT:       impl: [[OMPDD]]
    // DMP-OPRG-NEXT:         OMPSharedClause
    // DMP-OPRG-OSIMP-SAME:     <implicit>
    // DMP-OPRG-OSEXP-NOT:      <implicit>
    // DMP-OPRG-NEXT:           DeclRefExpr {{.*}} 'j' 'int'
    // DMP-OPRG-NEXT:           DeclRefExpr {{.*}} 'loopOnly' 'int'
    // DMP-OPRG-NEXT:           DeclRefExpr {{.*}} 'save'
    // DMP-OPRG-NEXT:           DeclRefExpr {{.*}} 'loopOnlyArr' 'int [1]'
    // DMP-OPRG:              ForStmt
    // DMP-OPLC-NEXT:       impl: [[OMPDD]]
    // DMP-ONT1-NEXT:         OMPNum_threadsClause
    // DMP-ONT1-NEXT:           IntegerLiteral {{.*}} 'int' 1
    // DMP-OPLC-NEXT:         OMPSharedClause
    // DMP-OPLC-OSIMP-SAME:     <implicit>
    // DMP-OPLC-OSEXP-NOT:      <implicit>
    // DMP-OPLC-NEXT:           DeclRefExpr {{.*}} 'j' 'int'
    // DMP-OPLC-NEXT:           DeclRefExpr {{.*}} 'loopOnly' 'int'
    // DMP-OPLC-NEXT:           DeclRefExpr {{.*}} 'save'
    // DMP-OPLC-NEXT:           DeclRefExpr {{.*}} 'loopOnlyArr' 'int [1]'
    // DMP-OPLC:              ForStmt
    // DMP-OSEQ-NEXT:       impl: ForStmt
    //
    // Print uncommented directive.
    // PRT-A-NEXT:            {{^ *}}#pragma acc loop[[ACCC]]
    // PRT-AO-OSEQ-SAME:      {{^}} // discarded in OpenMP translation
    // PRT-A-SAME:            {{^$}}
    // PRT-O-OPRG-OSIMP-NEXT: {{^ *}}#pragma omp [[OMPDP]]{{$}}
    // PRT-O-OPLC-OSIMP-NEXT: {{^ *}}#pragma omp [[OMPDP]]{{$}}
    // PRT-O-OPRG-OSEXP-NEXT: {{^ *}}#pragma omp [[OMPDP]] shared(j,loopOnly,save,loopOnlyArr){{$}}
    // PRT-O-OPLC-OSEXP-NEXT: {{^ *}}#pragma omp [[OMPDP]] shared(j,loopOnly,save,loopOnlyArr){{$}}
    //
    // Print commented directive.
    // PRT-AO-OPRG-OSIMP-NEXT: {{^ *}}// #pragma omp [[OMPDP]]{{$}}
    // PRT-AO-OPLC-OSIMP-NEXT: {{^ *}}// #pragma omp [[OMPDP]]{{$}}
    // PRT-AO-OPRG-OSEXP-NEXT: {{^ *}}// #pragma omp [[OMPDP]] shared(j,loopOnly,save,loopOnlyArr){{$}}
    // PRT-AO-OPLC-OSEXP-NEXT: {{^ *}}// #pragma omp [[OMPDP]] shared(j,loopOnly,save,loopOnlyArr){{$}}
    // PRT-OA-NEXT:            {{^ *}}// #pragma acc loop[[ACCC]]
    // PRT-OA-OSEQ-SAME:       {{^}} // discarded in OpenMP translation
    // PRT-OA-SAME:            {{^$}}
    //
    // Print attached statement.
    // PRT-NEXT: for
    #pragma acc loop ACCC
    for (int i = 0; i < 2; ++i) {
      // EXE:             hello world
      // EXE-NEXT:        hello world
      // EXE-GREDUN-NEXT: hello world
      // EXE-GREDUN-NEXT: hello world
      // EXE-GREDUN-NEXT: hello world
      // EXE-GREDUN-NEXT: hello world
      printf("hello world\n");
      for (j = 0; j < 2; ++j)
        ;

      // If gang-redundant, every gang should see old value at i = 0, new value
      // at i = 1 if sequential, and either value at i = 1 if otherwise
      // partitioned.  If gang-partitioned, each of two gangs should see old
      // value at the gang's one assigned iteration, and one gang shouldn't see
      // anything.
      if (loopOnly == 88) // check old before new in case assign between
        save.readLoopOnlyOld = true;
      else if (loopOnly == 77 && i == 1)
        save.readLoopOnlyNew = true;
      else
        save.readLoopOnlyErr = true;
      if (i == 0)
        loopOnly = 77;

      // At least one gang should see old value at i = 0, every gang should see
      // new value at i = 1 if sequential, and gangs can see either value (or
      // no value in case of the gang not assigned an iteration under gang
      // partitioning) at i = 1 if partitioned in any way.
      if (loopOnlyArr[0] == 88) // check old before new in case assign between
        save.readLoopOnlyArrOld = true;
      else if (loopOnlyArr[0] == 77)
        save.readLoopOnlyArrNew = true;
      else
        save.readLoopOnlyArrErr = true;
      if (i == 0)
        loopOnlyArr[0] = 77;
    }
    // Should see j value assigned inside loop except that, if
    // gang-partitioned, one of the three gangs won't run it.
    if (j == 0)
      save.declNestedLoopOld = true;
    else if (j == 2)
      save.declNestedLoopNew = true;
    else
      save.declNestedLoopErr = true;

    // Exercise assigned loop control variable, j, predetermined private when
    // the loop is partitioned, but implicitly shared when loop is sequential.
    //
    // Despite being a control variable on an inner loop, k shouldn't be
    // handled as the control variable for the enclosing acc loop and so
    // shouldn't be predetermined private.
    //
    // PRT: int jOld = j;
    int jOld = j;
    // DMP:              ACCLoopDirective
    // DMP-ASEQ-NEXT:      ACCSeqClause
    // DMP-AIND-NEXT:      ACCIndependentClause
    // DMP-AAUTO-NEXT:     ACCAutoClause
    // DMP-ASEQ-NOT:       <implicit>
    // DMP-AIND-NOT:       <implicit>
    // DMP-AAUTO-NOT:      <implicit>
    // DMP-AG-NEXT:        ACCGangClause
    // DMP-AW-NEXT:        ACCWorkerClause
    // DMP-AV-NEXT:        ACCVectorClause
    // DMP-AIMP-NEXT:      ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:           ACCSharedClause {{.*}} <implicit>
    // DMP-ASLC-NEXT:        DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:             DeclRefExpr {{.*}} 'k' 'int'
    // DMP-APLC-NEXT:      ACCPrivateClause {{.*}} <implicit>
    // DMP-APLC-NEXT:        DeclRefExpr {{.*}} 'j' 'int'
    // DMP-OPRG-NEXT:      impl: [[OMPDD]]
    // DMP-OPRG-NEXT:        OMPSharedClause
    // DMP-OPRG-OSIMP-SAME:    <implicit>
    // DMP-OPRG-OSEXP-NOT:     <implicit>
    // DMP-OPRG-NEXT:          DeclRefExpr {{.*}} 'k' 'int'
    // DMP-OPRG-NEXT:        OMPPrivateClause
    // DMP-OPRG-NOT:           <implicit>
    // DMP-OPRG-NEXT:          DeclRefExpr {{.*}} 'j' 'int'
    // DMP-OPRG:             ForStmt
    // DMP-OPLC-NEXT:      impl: CompoundStmt
    // DMP-OPLC-NEXT:        DeclStmt
    // DMP-OPLC-NEXT:          VarDecl {{.*}} j 'int'
    // DMP-OPLC-NEXT:        [[OMPDD]]
    // DMP-ONT1-NEXT:          OMPNum_threadsClause
    // DMP-ONT1-NEXT:            IntegerLiteral {{.*}} 'int' 1
    // DMP-OPLC:               ForStmt
    // DMP-OSEQ-NEXT:      impl: ForStmt
    //
    // Print uncommented directive.
    // PRT-A-NEXT:            {{^ *}}#pragma acc loop[[ACCC]]
    // PRT-AO-OSEQ-SAME:      {{^}} // discarded in OpenMP translation
    // PRT-A-SAME:            {{^$}}
    // PRT-O-OPRG-OSIMP-NEXT: {{^ *}}#pragma omp [[OMPDP]] private(j){{$}}
    // PRT-O-OPRG-OSEXP-NEXT: {{^ *}}#pragma omp [[OMPDP]] shared(k) private(j){{$}}
    //
    // Print commented directive when attached statements are the same.
    // PRT-AO-OPRG-OSIMP-NEXT: {{^ *}}// #pragma omp [[OMPDP]] private(j){{$}}
    // PRT-AO-OPRG-OSEXP-NEXT: {{^ *}}// #pragma omp [[OMPDP]] shared(k) private(j){{$}}
    // PRT-OA-OPRG-NEXT:       {{^ *}}// #pragma acc loop[[ACCC]]{{$}}
    // PRT-OA-OSEQ-NEXT:       {{^ *}}// #pragma acc loop[[ACCC]] // discarded in OpenMP translation{{$}}
    //
    // Print uncommented attached statement.
    // PRT-O-OPLC-NEXT:       {
    // PRT-O-OPLC-NEXT:         int j;
    // PRT-O-OPLC-OSIMP-NEXT:   {{^ *}}#pragma omp [[OMPDP]]{{$}}
    // PRT-O-OPLC-OSEXP-NEXT:   {{^ *}}#pragma omp [[OMPDP]] shared(k){{$}}
    // PRT-NEXT:                for (j ={{.*}}) {
    // PRT-NEXT:                  printf
    // PRT-NEXT:                  for (k ={{.*}})
    // PRT-NEXT:                    ;
    // PRT-NEXT:                }
    // PRT-O-OPLC-NEXT:       }
    //
    // Print commented directive and attached statement when attached
    // statements are different.
    // PRT-AOX-OPLC-NEXT:      /*
    // PRT-AO-OPLC-NEXT:       {
    // PRT-AO-OPLC-NEXT:         int j;
    // PRT-AO-OPLC-OSIMP-NEXT:   {{^ *}}#pragma omp [[OMPDP]]{{$}}
    // PRT-AO-OPLC-OSEXP-NEXT:   {{^ *}}#pragma omp [[OMPDP]] shared(k){{$}}
    // PRT-OA-OPLC-NEXT:         {{^ *}}#pragma acc loop[[ACCC]]{{$}}
    // PRT-AOX-OPLC-NEXT:        for (j ={{.*}}) {
    // PRT-AOX-OPLC-NEXT:          printf
    // PRT-AOX-OPLC-NEXT:          for (k ={{.*}})
    // PRT-AOX-OPLC-NEXT:            ;
    // PRT-AOX-OPLC-NEXT:        }
    // PRT-AO-OPLC-NEXT:       }
    // PRT-AOX-OPLC-NEXT:      */
    #pragma acc loop ACCC
    for (j = 1; j < 3; ++j) {
      // EXE-NEXT:        hello world
      // EXE-NEXT:        hello world
      // EXE-GREDUN-NEXT: hello world
      // EXE-GREDUN-NEXT: hello world
      // EXE-GREDUN-NEXT: hello world
      // EXE-GREDUN-NEXT: hello world
      printf("hello world\n");
      for (k = 0; k < 2; ++k)
        ;
    }
    // If sequential or gang-redundant plus vector-partitioned, should see new
    // j value.  If partitioned but not vector-partitioned, should see old j
    // value.  If gang-partitioned and vector-partitioned, only one gang will
    // see the new j value, and the rest will see the old j value.
    if (j == jOld)
      save.assignLoopOld = true;
    else if (j == 3)
      save.assignLoopNew = true;
    else
      save.assignLoopErr = true;
    // Should see k value assigned inside loop except that, if
    // gang-partitioned, one of the three gangs won't run it.
    if (k == 0)
      save.assignNestedLoopOld = true;
    else if (k == 2)
      save.assignNestedLoopNew = true;
    else
      save.assignNestedLoopErr = true;

    // j is used here but it should have been dropped from the loop control
    // variable list, so it should now be implicitly shared in all cases.
    //
    // PRT: jOld = j;
    jOld = j;
    // DMP:               ACCLoopDirective
    // DMP-ASEQ-NEXT:       ACCSeqClause
    // DMP-AIND-NEXT:       ACCIndependentClause
    // DMP-AAUTO-NEXT:      ACCAutoClause
    // DMP-ASEQ-NOT:        <implicit>
    // DMP-AIND-NOT:        <implicit>
    // DMP-AAUTO-NOT:       <implicit>
    // DMP-AG-NEXT:         ACCGangClause
    // DMP-AW-NEXT:         ACCWorkerClause
    // DMP-AV-NEXT:         ACCVectorClause
    // DMP-AIMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:            ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:              DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:              DeclRefExpr {{.*}} 'jOld' 'int'
    // DMP-NEXT:              DeclRefExpr {{.*}} 'save'
    // DMP-OPRG-NEXT:       impl: [[OMPDD]]
    // DMP-OPRG-NEXT:         OMPSharedClause
    // DMP-OPRG-OSIMP-SAME:     <implicit>
    // DMP-OPRG-OSEXP-NOT:      <implicit>
    // DMP-OPRG-NEXT:           DeclRefExpr {{.*}} 'j' 'int'
    // DMP-OPRG-NEXT:           DeclRefExpr {{.*}} 'jOld' 'int'
    // DMP-OPRG-NEXT:           DeclRefExpr {{.*}} 'save'
    // DMP-OPRG:              ForStmt
    // DMP-OPLC-NEXT:       impl: [[OMPDD]]
    // DMP-ONT1-NEXT:         OMPNum_threadsClause
    // DMP-ONT1-NEXT:           IntegerLiteral {{.*}} 'int' 1
    // DMP-OPLC-NEXT:         OMPSharedClause
    // DMP-OPLC-OSIMP-SAME:     <implicit>
    // DMP-OPLC-OSEXP-NOT:      <implicit>
    // DMP-OPLC-NEXT:           DeclRefExpr {{.*}} 'j' 'int'
    // DMP-OPLC-NEXT:           DeclRefExpr {{.*}} 'jOld' 'int'
    // DMP-OPLC-NEXT:           DeclRefExpr {{.*}} 'save'
    // DMP-OPLC:              ForStmt
    // DMP-OSEQ-NEXT:       impl: ForStmt
    //
    // Print uncommented directive.
    // PRT-A-NEXT:            {{^ *}}#pragma acc loop[[ACCC]]
    // PRT-AO-OSEQ-SAME:      {{^}} // discarded in OpenMP translation
    // PRT-A-SAME:            {{^$}}
    // PRT-O-OPRG-OSIMP-NEXT: {{^ *}}#pragma omp [[OMPDP]]{{$}}
    // PRT-O-OPLC-OSIMP-NEXT: {{^ *}}#pragma omp [[OMPDP]]{{$}}
    // PRT-O-OPRG-OSEXP-NEXT: {{^ *}}#pragma omp [[OMPDP]] shared(j,jOld,save){{$}}
    // PRT-O-OPLC-OSEXP-NEXT: {{^ *}}#pragma omp [[OMPDP]] shared(j,jOld,save){{$}}
    //
    // Print commented directive.
    // PRT-AO-OPRG-OSIMP-NEXT: {{^ *}}// #pragma omp [[OMPDP]]{{$}}
    // PRT-AO-OPLC-OSIMP-NEXT: {{^ *}}// #pragma omp [[OMPDP]]{{$}}
    // PRT-AO-OPRG-OSEXP-NEXT: {{^ *}}// #pragma omp [[OMPDP]] shared(j,jOld,save){{$}}
    // PRT-AO-OPLC-OSEXP-NEXT: {{^ *}}// #pragma omp [[OMPDP]] shared(j,jOld,save){{$}}
    // PRT-OA-NEXT:            {{^ *}}// #pragma acc loop[[ACCC]]
    // PRT-OA-OSEQ-SAME:       {{^}} // discarded in OpenMP translation
    // PRT-OA-SAME:            {{^$}}
    //
    // Print attached statement.
    // PRT-NEXT: for
    #pragma acc loop ACCC
    for (int i = 0; i < 2; ++i) {
      // If gang-redundant, every gang should see old j value at i = 0, new
      // j value at i = 1 if sequential, and either j value at i = 1 if
      // otherwise partitioned.  If gang-partitioned, each of two gangs should
      // see old j value at the gang's one assigned iteration, and one gang
      // shouldn't see anything.
      if (j == jOld) // check old before new in case assign between
        save.readInLaterLoopOld = true;
      else if (j == 9999 && i == 1)
        save.readInLaterLoopNew = true;
      else
        save.readInLaterLoopErr = true;
      if (i == 0)
        j = 9999;
      // EXE-NEXT:        hello world
      // EXE-NEXT:        hello world
      // EXE-GREDUN-NEXT: hello world
      // EXE-GREDUN-NEXT: hello world
      // EXE-GREDUN-NEXT: hello world
      // EXE-GREDUN-NEXT: hello world
      printf("hello world\n");
    }

    // Should see j value assigned inside loop except that, if
    // gang-partitioned, one of the three gangs won't run it.
    if (j == jOld)
      save.writeInLaterLoopOld = true;
    else if (j == 9999)
      save.writeInLaterLoopNew = true;
    else
      save.writeInLaterLoopErr = true;
  // PRT: }
  }

  // EXE-GREDUN-NEXT: save.declNestedLoopOld=0
  // EXE-GPART-NEXT:  save.declNestedLoopOld=1
  printf("save.declNestedLoopOld=%d\n", save.declNestedLoopOld);
  // EXE-NEXT: save.declNestedLoopNew=1
  printf("save.declNestedLoopNew=%d\n", save.declNestedLoopNew);
  // EXE-NEXT: save.declNestedLoopErr=0
  printf("save.declNestedLoopErr=%d\n", save.declNestedLoopErr);

  // EXE-SLC-NEXT:        save.assignLoopOld=0
  // EXE-PLC-NEXT:        save.assignLoopOld=1
  printf("save.assignLoopOld=%d\n", save.assignLoopOld);
  // EXE-SLC-NEXT:        save.assignLoopNew=1
  // EXE-PLC-NEXT:        save.assignLoopNew=0
  printf("save.assignLoopNew=%d\n", save.assignLoopNew);
  // EXE-NEXT: save.assignLoopErr=0
  printf("save.assignLoopErr=%d\n", save.assignLoopErr);

  // EXE-GREDUN-NEXT: save.assignNestedLoopOld=0
  // EXE-GPART-NEXT:  save.assignNestedLoopOld=1
  printf("save.assignNestedLoopOld=%d\n", save.assignNestedLoopOld);
  // EXE-NEXT: save.assignNestedLoopNew=1
  printf("save.assignNestedLoopNew=%d\n", save.assignNestedLoopNew);
  // EXE-NEXT: save.assignNestedLoopErr=0
  printf("save.assignNestedLoopErr=%d\n", save.assignNestedLoopErr);

  // EXE-NEXT: save.readInLaterLoopOld=1
  printf("save.readInLaterLoopOld=%d\n", save.readInLaterLoopOld);
  // EXE-NOPART-NEXT: save.readInLaterLoopNew=1
  // EXE-GPART-NEXT:  save.readInLaterLoopNew=0
  printf("save.readInLaterLoopNew=%d\n", save.readInLaterLoopNew);
  // We don't have a prefix for gang-redundant but partitioned, and the value
  // is indeterminate there anyway, so we skip that case.
  // EXE: save.readInLaterLoopErr=0
  printf("save.readInLaterLoopErr=%d\n", save.readInLaterLoopErr);

  // EXE-GREDUN-NEXT: save.writeInLaterLoopOld=0
  // EXE-GPART-NEXT:  save.writeInLaterLoopOld=1
  printf("save.writeInLaterLoopOld=%d\n", save.writeInLaterLoopOld);
  // EXE-NEXT: save.writeInLaterLoopNew=1
  printf("save.writeInLaterLoopNew=%d\n", save.writeInLaterLoopNew);
  // EXE-NEXT: save.writeInLaterLoopErr=0
  printf("save.writeInLaterLoopErr=%d\n", save.writeInLaterLoopErr);

  // EXE-NEXT: save.readLoopOnlyOld=1
  printf("save.readLoopOnlyOld=%d\n", save.readLoopOnlyOld);
  // EXE-NOPART-NEXT: save.readLoopOnlyNew=1
  // EXE-GPART-NEXT:  save.readLoopOnlyNew=0
  printf("save.readLoopOnlyNew=%d\n", save.readLoopOnlyNew);
  // We don't have a prefix for gang-redundant but partitioned, and the value
  // is indeterminate there anyway, so we skip that case.
  // EXE: save.readLoopOnlyErr=0
  printf("save.readLoopOnlyErr=%d\n", save.readLoopOnlyErr);

  // EXE-NEXT: loopOnly=88
  printf("loopOnly=%d\n", loopOnly);

  // EXE-NEXT: save.readLoopOnlyArrOld=1
  printf("save.readLoopOnlyArrOld=%d\n", save.readLoopOnlyArrOld);
  // EXE-NOPART-NEXT: save.readLoopOnlyArrNew=1
  // EXE-PART-NEXT: save.readLoopOnlyArrNew={{[01]}}
  printf("save.readLoopOnlyArrNew=%d\n", save.readLoopOnlyArrNew);
  // EXE-NEXT: save.readLoopOnlyArrErr=0
  printf("save.readLoopOnlyArrErr=%d\n", save.readLoopOnlyArrErr);

  // EXE-NEXT: loopOnlyArr[0]=77
  // PRT: printf("loopOnlyArr[0]=%d\n", loopOnlyArr[0]);
  printf("loopOnlyArr[0]=%d\n", loopOnlyArr[0]);

  // A sequential loop should be able to affect its number of iterations when
  // modifying its control variable in its body.  A partitioned loop just uses
  // the number of iterations specified in the for init, cond, and incr.
  // DMP:      ACCParallelDirective
  // DMP-NEXT:   impl: OMPTargetTeamsDirective
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc parallel{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams{{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp target teams{{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel{{$}}
  #pragma acc parallel
  // DMP:             ACCLoopDirective
  // DMP-ASEQ-NEXT:     ACCSeqClause
  // DMP-AIND-NEXT:     ACCIndependentClause
  // DMP-AAUTO-NEXT:    ACCAutoClause
  // DMP-ASEQ-NOT:      <implicit>
  // DMP-AIND-NOT:      <implicit>
  // DMP-AAUTO-NOT:     <implicit>
  // DMP-AG-NEXT:       ACCGangClause
  // DMP-AW-NEXT:       ACCWorkerClause
  // DMP-AV-NEXT:       ACCVectorClause
  // DMP-AIMP-NEXT:     ACCIndependentClause {{.*}} <implicit>
  // DMP-OPRG-NEXT:     impl: [[OMPDD]]
  // DMP-OPRG:            ForStmt
  // DMP-OPLC-NEXT:     impl: [[OMPDD]]
  // DMP-ONT1-NEXT:       OMPNum_threadsClause
  // DMP-ONT1-NEXT:         IntegerLiteral {{.*}} 'int' 1
  // DMP-OPLC:            ForStmt
  // DMP-OSEQ-NEXT:     impl: ForStmt
  //
  // Print uncommented directive.
  // PRT-A-NEXT:       {{^ *}}#pragma acc loop[[ACCC]]
  // PRT-AO-OSEQ-SAME: {{^}} // discarded in OpenMP translation
  // PRT-A-SAME:       {{^$}}
  // PRT-O-OPRG-NEXT:  {{^ *}}#pragma omp [[OMPDP]]{{$}}
  // PRT-O-OPLC-NEXT:  {{^ *}}#pragma omp [[OMPDP]]{{$}}
  //
  // Print commented directive.
  // PRT-AO-OPRG-NEXT: {{^ *}}// #pragma omp [[OMPDP]]{{$}}
  // PRT-AO-OPLC-NEXT: {{^ *}}// #pragma omp [[OMPDP]]{{$}}
  // PRT-OA-NEXT:      {{^ *}}// #pragma acc loop[[ACCC]]
  // PRT-OA-OSEQ-SAME: {{^}} // discarded in OpenMP translation
  // PRT-OA-SAME:      {{^$}}
  //
  // Print attached statement.
  // PRT-NEXT: for
  #pragma acc loop ACCC
  for (int i = 0; i < 2; ++i) {
    // EXE-NEXT:      just once
    // EXE-PART-NEXT: just once
    printf("just once\n");
    i = 2;
  // PRT: }
  }

  // Make sure that an init!=0 or inc!=1 work correctly.
  // The latter is specifically relevant to simd directives, which depend on
  // OpenMP's predetermined linear attribute with a step that is the loop inc.
  // DMP:      ACCParallelDirective
  // DMP-NEXT:   impl: OMPTargetTeamsDirective
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc parallel{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams{{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp target teams{{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel{{$}}
  #pragma acc parallel
  {
    // PRT: int j;
    int j;
    // DMP:               ACCLoopDirective
    // DMP-ASEQ-NEXT:       ACCSeqClause
    // DMP-AIND-NEXT:       ACCIndependentClause
    // DMP-AAUTO-NEXT:      ACCAutoClause
    // DMP-ASEQ-NOT:        <implicit>
    // DMP-AIND-NOT:        <implicit>
    // DMP-AAUTO-NOT:       <implicit>
    // DMP-AG-NEXT:         ACCGangClause
    // DMP-AW-NEXT:         ACCWorkerClause
    // DMP-AV-NEXT:         ACCVectorClause
    // DMP-AIMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
    // DMP-ASLC-NEXT:       ACCSharedClause {{.*}} <implicit>
    // DMP-ASLC-NEXT:         DeclRefExpr {{.*}} 'j' 'int'
    // DMP-APLC-NEXT:       ACCPrivateClause {{.*}} <implicit>
    // DMP-APLC-NEXT:         DeclRefExpr {{.*}} 'j' 'int'
    // DMP-OPRG-NEXT:       impl: [[OMPDD]]
    // DMP-OPRG-NEXT:         OMPPrivateClause
    // DMP-OPRG-NOT:            <implicit>
    // DMP-OPRG-NEXT:           DeclRefExpr {{.*}} 'j' 'int'
    // DMP-OPRG:              ForStmt
    // DMP-OPLC-NEXT:       impl: CompoundStmt
    // DMP-OPLC-NEXT:         DeclStmt
    // DMP-OPLC-NEXT:           VarDecl {{.*}} j 'int'
    // DMP-OPLC-NEXT:         [[OMPDD]]
    // DMP-ONT1-NEXT:           OMPNum_threadsClause
    // DMP-ONT1-NEXT:             IntegerLiteral {{.*}} 'int' 1
    // DMP-OPLC:                ForStmt
    // DMP-OSEQ-NEXT:       impl: ForStmt
    //
    // Print uncommented directive.
    // PRT-A-NEXT:       {{^ *}}#pragma acc loop[[ACCC]]
    // PRT-AO-OSEQ-SAME: {{^}} // discarded in OpenMP translation
    // PRT-A-SAME:       {{^$}}
    // PRT-O-OPRG-NEXT:  {{^ *}}#pragma omp [[OMPDP]] private(j){{$}}
    //
    // Print commented directive and attached statement when attached
    // statements are different.
    // PRT-AO-OPRG-NEXT: {{^ *}}// #pragma omp [[OMPDP]] private(j){{$}}
    // PRT-OA-OPRG-NEXT: {{^ *}}// #pragma acc loop[[ACCC]]{{$}}
    // PRT-OA-OSEQ-NEXT: {{^ *}}// #pragma acc loop[[ACCC]] // discarded in OpenMP translation{{$}}
    //
    // Print uncommented attached statement.
    // PRT-O-OPLC-NEXT: {
    // PRT-O-OPLC-NEXT:   int j;
    // PRT-O-OPLC-NEXT:   {{^ *}}#pragma omp [[OMPDP]]{{$}}
    // PRT-NEXT:          for (j ={{.*}}) {
    // PRT-NEXT:            printf
    // PRT-NEXT:          }
    // PRT-O-OPLC-NEXT: }
    //
    // Print commented directive and attached statement when attached
    // statements are different.
    // PRT-AOX-OPLC-NEXT: /*
    // PRT-AO-OPLC-NEXT:  {
    // PRT-AO-OPLC-NEXT:    int j;
    // PRT-AO-OPLC-NEXT:    {{^ *}}#pragma omp [[OMPDP]]{{$}}
    // PRT-OA-OPLC-NEXT:    {{^ *}}#pragma acc loop[[ACCC]]{{$}}
    // PRT-AOX-OPLC-NEXT:   for (j ={{.*}}) {
    // PRT-AOX-OPLC-NEXT:     printf
    // PRT-AOX-OPLC-NEXT:   }
    // PRT-AO-OPLC-NEXT:  }
    // PRT-AOX-OPLC-NEXT: */
    #pragma acc loop ACCC
    for (j = 7; j > -2; j -= 2) {
      // EXE-DAG: 7
      // EXE-DAG: 5
      // EXE-DAG: 3
      // EXE-DAG: {{^1}}
      // EXE-DAG: -1
      printf("%d\n", j);
    }
  }

  // Close off EXE-DAGs.
  // EXE-NOT: {{.}}
  // EXE-NEXT: END
  printf("END\n");

  return 0;
  // EXE-NOT: {{.}}
}
