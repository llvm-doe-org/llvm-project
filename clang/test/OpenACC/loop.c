// Check correct loop partitioning and implicit data sharing on "acc loop"
// and "acc parallel loop".
//
// Abbreviations:
//   A       = OpenACC
//   AO      = commented OpenMP is printed after OpenACC
//   O       = OpenMP
//   OA      = commented OpenACC is printed after OpenMP
//   AIMP    = OpenACC implicit independent
//   AIND    = OpenACC explicit independent clause
//   ASEQ    = OpenACC seq clause
//   AAUTO   = OpenACC auto clause
//   AG      = OpenACC explicit gang clause
//   AGIMP   = OpenACC implicit gang clause
//   AW      = OpenACC worker clause
//   AV      = OpenACC vector clause
//   ASLC    = OpenACC shared clause for assigned loop control variable
//   APLC    = OpenACC private clause for assigned loop control variable
//   OPRG    = OpenACC pragma translated to OpenMP pragma
//   OPRGPLC = OPRG but any assigned loop control var becomes declared private
//             in an enclosing compound statement
//   OSEQ    = OpenACC loop seq discarded in translation to OpenMP
//   OSEQPLC = OSEQ but any assigned loop control var becomes declared private
//             in an enclosing compound statement
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
//   ompdk   = OpenMP directive kind (OPRG, OPRGPLC, OSEQ, OSEQPLC)
//   ompsk   = OpenMP shared kind (OSIMP or OSEXP)
//   dmp     = additional FileCheck prefixes for dump
//   exe     = additional FileCheck prefixes for execution
//
// RUN: %data loop-clauses {
//        implicit independent, seq, independent, auto
// RUN:   (accc=
// RUN:    accc_sp=
// RUN:    ompdd=OMPDistributeDirective
// RUN:    ompdp=distribute
// RUN:    ompdk=OPRG
// RUN:    ompsk=OSIMP
// RUN:    dmp=DMP-AIMP,DMP-APLC,DMP-AGIMP
// RUN:    exe=EXE,EXE-PART,EXE-GPART,EXE-PLC)
// RUN:   (accc=seq
// RUN:    accc_sp=' '
// RUN:    ompdd=
// RUN:    ompdp=
// RUN:    ompdk=OSEQ
// RUN:    ompsk=OSIMP
// RUN:    dmp=DMP-ASEQ,DMP-ASLC
// RUN:    exe=EXE,EXE-NOPART,EXE-GREDUN,EXE-NOPART-GREDUN,EXE-SLC)
// RUN:   (accc=independent
// RUN:    accc_sp=' '
// RUN:    ompdd=OMPDistributeDirective
// RUN:    ompdp=distribute
// RUN:    ompdk=OPRG
// RUN:    ompsk=OSIMP
// RUN:    dmp=DMP-AIND,DMP-APLC,DMP-AGIMP
// RUN:    exe=EXE,EXE-PART,EXE-GPART,EXE-PLC)
// RUN:   (accc=auto
// RUN:    accc_sp=' '
// RUN:    ompdd=
// RUN:    ompdp=
// RUN:    ompdk=OSEQPLC
// RUN:    ompsk=OSIMP
// RUN:    dmp=DMP-AAUTO,DMP-APLC
// RUN:    exe=EXE,EXE-NOPART,EXE-GREDUN,EXE-NOPART-GREDUN,EXE-PLC)
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
// RUN:    exe=EXE,EXE-PART,EXE-GREDUN,EXE-PART-GREDUN,EXE-PLC)
// RUN:   (accc=vector
// RUN:    accc_sp=' '
// RUN:    ompdd=OMPParallelForSimdDirective
// RUN:    ompdp='parallel for simd num_threads(1)'
// RUN:    ompdk=OPRGPLC
// RUN:    ompsk=OSEXP
// RUN:    dmp=DMP-AIMP,DMP-APLC,DMP-AV,DMP-ONT1
// RUN:    exe=EXE,EXE-PART,EXE-GREDUN,EXE-PART-GREDUN,EXE-PLC)
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
// RUN:    exe=EXE,EXE-PART,EXE-GREDUN,EXE-PART-GREDUN,EXE-PLC)
// RUN:   (accc='independent vector'
// RUN:    accc_sp=' '
// RUN:    ompdd=OMPParallelForSimdDirective
// RUN:    ompdp='parallel for simd num_threads(1)'
// RUN:    ompdk=OPRGPLC
// RUN:    ompsk=OSEXP
// RUN:    dmp=DMP-AIND,DMP-APLC,DMP-AV,DMP-ONT1
// RUN:    exe=EXE,EXE-PART,EXE-GREDUN,EXE-PART-GREDUN,EXE-PLC)
// RUN:   (accc='auto gang'
// RUN:    accc_sp=' '
// RUN:    ompdd=
// RUN:    ompdp=
// RUN:    ompdk=OSEQPLC
// RUN:    ompsk=OSIMP
// RUN:    dmp=DMP-AAUTO,DMP-APLC,DMP-AG
// RUN:    exe=EXE,EXE-NOPART,EXE-GREDUN,EXE-NOPART-GREDUN,EXE-PLC)
// RUN:   (accc='auto worker'
// RUN:    accc_sp=' '
// RUN:    ompdd=
// RUN:    ompdp=
// RUN:    ompdk=OSEQPLC
// RUN:    ompsk=OSIMP
// RUN:    dmp=DMP-AAUTO,DMP-APLC,DMP-AW
// RUN:    exe=EXE,EXE-NOPART,EXE-GREDUN,EXE-NOPART-GREDUN,EXE-PLC)
// RUN:   (accc='auto vector'
// RUN:    accc_sp=' '
// RUN:    ompdd=
// RUN:    ompdp=
// RUN:    ompdk=OSEQPLC
// RUN:    ompsk=OSIMP
// RUN:    dmp=DMP-AAUTO,DMP-APLC,DMP-AV
// RUN:    exe=EXE,EXE-NOPART,EXE-GREDUN,EXE-NOPART-GREDUN,EXE-PLC)
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
// RUN:    ompdk=OPRGPLC
// RUN:    ompsk=OSIMP
// RUN:    dmp=DMP-AIMP,DMP-APLC,DMP-AG,DMP-AV
// RUN:    exe=EXE,EXE-PART,EXE-GPART,EXE-PLC)
// RUN:   (accc='independent gang vector'
// RUN:    accc_sp=' '
// RUN:    ompdd=OMPDistributeSimdDirective
// RUN:    ompdp='distribute simd'
// RUN:    ompdk=OPRGPLC
// RUN:    ompsk=OSIMP
// RUN:    dmp=DMP-AIND,DMP-APLC,DMP-AG,DMP-AV
// RUN:    exe=EXE,EXE-PART,EXE-GPART,EXE-PLC)
// RUN:   (accc='worker vector'
// RUN:    accc_sp=' '
// RUN:    ompdd=OMPParallelForSimdDirective
// RUN:    ompdp='parallel for simd'
// RUN:    ompdk=OPRGPLC
// RUN:    ompsk=OSEXP
// RUN:    dmp=DMP-AIMP,DMP-APLC,DMP-AW,DMP-AV
// RUN:    exe=EXE,EXE-PART,EXE-GREDUN,EXE-PART-GREDUN,EXE-PLC)
// RUN:   (accc='independent worker vector'
// RUN:    accc_sp=' '
// RUN:    ompdd=OMPParallelForSimdDirective
// RUN:    ompdp='parallel for simd'
// RUN:    ompdk=OPRGPLC
// RUN:    ompsk=OSEXP
// RUN:    dmp=DMP-AIND,DMP-APLC,DMP-AW,DMP-AV
// RUN:    exe=EXE,EXE-PART,EXE-GREDUN,EXE-PART-GREDUN,EXE-PLC)
// RUN:   (accc='gang worker vector'
// RUN:    accc_sp=' '
// RUN:    ompdd=OMPDistributeParallelForSimdDirective
// RUN:    ompdp='distribute parallel for simd'
// RUN:    ompdk=OPRGPLC
// RUN:    ompsk=OSEXP
// RUN:    dmp=DMP-AIMP,DMP-APLC,DMP-AG,DMP-AW,DMP-AV
// RUN:    exe=EXE,EXE-PART,EXE-GPART,EXE-PLC)
// RUN:   (accc='independent gang worker vector'
// RUN:    accc_sp=' '
// RUN:    ompdd=OMPDistributeParallelForSimdDirective
// RUN:    ompdp='distribute parallel for simd'
// RUN:    ompdk=OPRGPLC
// RUN:    ompsk=OSEXP
// RUN:    dmp=DMP-AIND,DMP-APLC,DMP-AG,DMP-AW,DMP-AV
// RUN:    exe=EXE,EXE-PART,EXE-GPART,EXE-PLC)
// RUN:   (accc='auto gang worker'
// RUN:    accc_sp=' '
// RUN:    ompdd=
// RUN:    ompdp=
// RUN:    ompdk=OSEQPLC
// RUN:    ompsk=OSIMP
// RUN:    dmp=DMP-AAUTO,DMP-APLC,DMP-AG,DMP-AW
// RUN:    exe=EXE,EXE-NOPART,EXE-GREDUN,EXE-NOPART-GREDUN,EXE-PLC)
// RUN:   (accc='auto gang vector'
// RUN:    accc_sp=' '
// RUN:    ompdd=
// RUN:    ompdp=
// RUN:    ompdk=OSEQPLC
// RUN:    ompsk=OSIMP
// RUN:    dmp=DMP-AAUTO,DMP-APLC,DMP-AG,DMP-AV
// RUN:    exe=EXE,EXE-NOPART,EXE-GREDUN,EXE-NOPART-GREDUN,EXE-PLC)
// RUN:   (accc='auto worker vector'
// RUN:    accc_sp=' '
// RUN:    ompdd=
// RUN:    ompdp=
// RUN:    ompdk=OSEQPLC
// RUN:    ompsk=OSIMP
// RUN:    dmp=DMP-AAUTO,DMP-APLC,DMP-AW,DMP-AV
// RUN:    exe=EXE,EXE-NOPART,EXE-GREDUN,EXE-NOPART-GREDUN,EXE-PLC)
// RUN:   (accc='auto gang worker vector'
// RUN:    accc_sp=' '
// RUN:    ompdd=
// RUN:    ompdp=
// RUN:    ompdk=OSEQPLC
// RUN:    ompsk=OSIMP
// RUN:    dmp=DMP-AAUTO,DMP-APLC,DMP-AG,DMP-AW,DMP-AV
// RUN:    exe=EXE,EXE-NOPART,EXE-GREDUN,EXE-NOPART-GREDUN,EXE-PLC)
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

// Check -ast-print and -fopenacc[-ast]-print.
//
// RUN: %clang -Xclang -verify -Xclang -ast-print -fsyntax-only %s \
// RUN: | FileCheck -check-prefixes=PRT,PRT-NOACC %s
//
// Strip comments and blank lines so checking -fopenacc-print output is easier.
// RUN: echo "// expected""-no-diagnostics" > %t-acc.c
// RUN: grep -v '^ *\(//.*\)\?$' %s | sed 's,//.*,,' >> %t-acc.c
//
// TODO: If lit were to support %for inside a %data, we could iterate prt-opts
// (which would need additional fields) within prt-args after the first
// prt-args iteration, significantly shortening the prt-args definition.
//
// RUN: %data prt-opts {
// RUN:   (prt-opt=-fopenacc-ast-print)
// RUN:   (prt-opt=-fopenacc-print    )
// RUN: }
// RUN: %data prt-args {
// RUN:   (prt='-Xclang -ast-print -fsyntax-only -fopenacc' ACCC="%'accc_sp'%'accc'" prt-chk=PRT,PRT-A)
// RUN:   (prt=-fopenacc-ast-print=acc                      ACCC="%'accc_sp'%'accc'" prt-chk=PRT,PRT-A)
// RUN:   (prt=-fopenacc-ast-print=omp                      ACCC="%'accc_sp'%'accc'" prt-chk=PRT,PRT-O,PRT-O-%[ompdk],PRT-O-%[ompdk]-%[ompsk])
// RUN:   (prt=-fopenacc-ast-print=acc-omp                  ACCC="%'accc_sp'%'accc'" prt-chk=PRT,PRT-A,PRT-AO,PRT-AO-%[ompdk],PRT-AO-%[ompdk]-%[ompsk])
// RUN:   (prt=-fopenacc-ast-print=omp-acc                  ACCC="%'accc_sp'%'accc'" prt-chk=PRT,PRT-O,PRT-O-%[ompdk],PRT-O-%[ompdk]-%[ompsk],PRT-OA,PRT-OA-%[ompdk],PRT-OA-%[ompdk]-%[ompsk])
// RUN:   (prt=-fopenacc-print=acc                          ACCC="' ACCC'"           prt-chk=PRT,PRT-A)
// RUN:   (prt=-fopenacc-print=omp                          ACCC="' ACCC'"           prt-chk=PRT,PRT-O,PRT-O-%[ompdk],PRT-O-%[ompdk]-%[ompsk])
// RUN:   (prt=-fopenacc-print=acc-omp                      ACCC="' ACCC'"           prt-chk=PRT,PRT-A,PRT-AO,PRT-AO-%[ompdk],PRT-AO-%[ompdk]-%[ompsk])
// RUN:   (prt=-fopenacc-print=omp-acc                      ACCC="' ACCC'"           prt-chk=PRT,PRT-O,PRT-O-%[ompdk],PRT-O-%[ompdk]-%[ompsk],PRT-OA,PRT-OA-%[ompdk],PRT-OA-%[ompdk]-%[ompsk])
// RUN: }
// RUN: %for loop-clauses {
// RUN:   %for prt-args {
// RUN:     %clang -Xclang -verify %[prt] -DACCC=%'accc' %t-acc.c \
// RUN:     | FileCheck -check-prefixes=%[prt-chk] -DACCC=%[ACCC] \
// RUN:                 -DOMPDP=%'ompdp' %s
// RUN:   }
// RUN: }

// Check ASTWriterStmt, ASTReaderStmt, StmtPrinter, and
// ACCLoopDirective::CreateEmpty (used by ASTReaderStmt).  Some data related to
// printing (where to print comments about discarded directives) is serialized
// and deserialized, so it's worthwhile to try all OpenACC printing modes.
//
// RUN: %for loop-clauses {
// RUN:   %clang -Xclang -verify -fopenacc -emit-ast %t-acc.c -DACCC=%'accc' \
// RUN:          -o %t.ast
// RUN:   %for prt-args {
// RUN:     %clang %[prt] %t.ast 2>&1 \
// RUN:     | FileCheck -check-prefixes=%[prt-chk] -DACCC=%[ACCC] \
// RUN:                 -DOMPDP=%'ompdp' %s
// RUN:   }
// RUN: }

// Can we print the OpenMP source code, compile, and run it successfully?
//
// RUN: %for loop-clauses {
// RUN:   %for prt-opts {
// RUN:     %clang -Xclang -verify %[prt-opt]=omp %s > %t-omp.c \
// RUN:            -DACCC=%'accc'
// RUN:     echo "// expected""-no-diagnostics" >> %t-omp.c
// RUN:     %clang -Xclang -verify -fopenmp -o %t %t-omp.c \
// RUN:            -DACCC=%'accc'
// RUN:     %t | FileCheck -check-prefixes=%[exe] %s
// RUN:   }
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

// PRT: int tentativeDef;
int tentativeDef;

// PRT-NEXT: int main() {
int main() {

  //--------------------------------------------------
  // Exercise declared loop control variable, i.
  //
  // Despite being a control variable on an inner loop, j shouldn't be
  // handled as the control variable for the enclosing acc loop and so
  // shouldn't be predetermined private.
  //--------------------------------------------------

  // PRT-NEXT: {
  {
    // Accessed in acc loop but not otherwise in acc parallel.
    // PRT-NEXT: int loopOnly = 88;
    // PRT-NEXT: int loopOnlyArr[1] = {88};
    int loopOnly = 88;
    int loopOnlyArr[1] = {88};

    // PRT-NEXT: struct {
    struct {
      bool readLoopOnlyOld;
      bool readLoopOnlyNew;
      bool readLoopOnlyErr;

      bool readLoopOnlyArrOld;
      bool readLoopOnlyArrNew;
      bool readLoopOnlyArrErr;

      bool declNestedLoopOld;
      bool declNestedLoopNew;
      bool declNestedLoopErr;
    } save; // PRT: } save;

    // PRT-NEXT: memset
    memset(&save, 0, sizeof(save));

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
      int j = 0;

      // DMP:                  ACCLoopDirective
      // DMP-ASEQ-NEXT:          ACCSeqClause
      // DMP-AIND-NEXT:          ACCIndependentClause
      // DMP-AAUTO-NEXT:         ACCAutoClause
      // DMP-ASEQ-NOT:           <implicit>
      // DMP-AIND-NOT:           <implicit>
      // DMP-AAUTO-NOT:          <implicit>
      // DMP-AG-NEXT:            ACCGangClause
      // DMP-AG-NOT:             <implicit>
      // DMP-AW-NEXT:            ACCWorkerClause
      // DMP-AV-NEXT:            ACCVectorClause
      // DMP-AIMP-NEXT:          ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:               ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:                 DeclRefExpr {{.*}} 'j' 'int'
      // DMP-NEXT:                 DeclRefExpr {{.*}} 'loopOnly' 'int'
      // DMP-NEXT:                 DeclRefExpr {{.*}} 'save'
      // DMP-NEXT:                 DeclRefExpr {{.*}} 'loopOnlyArr' 'int [1]'
      // DMP-AGIMP-NEXT:         ACCGangClause {{.*}} <implicit>
      // DMP-OPRG-NEXT:          impl: [[OMPDD]]
      // DMP-OPRG-NEXT:            OMPSharedClause
      // DMP-OPRG-OSIMP-SAME:        <implicit>
      // DMP-OPRG-OSEXP-NOT:         <implicit>
      // DMP-OPRG-NEXT:              DeclRefExpr {{.*}} 'j' 'int'
      // DMP-OPRG-NEXT:              DeclRefExpr {{.*}} 'loopOnly' 'int'
      // DMP-OPRG-NEXT:              DeclRefExpr {{.*}} 'save'
      // DMP-OPRG-NEXT:              DeclRefExpr {{.*}} 'loopOnlyArr' 'int [1]'
      // DMP-OPRG:                 ForStmt
      // DMP-OPRGPLC-NEXT:       impl: [[OMPDD]]
      // DMP-ONT1-NEXT:            OMPNum_threadsClause
      // DMP-ONT1-NEXT:              IntegerLiteral {{.*}} 'int' 1
      // DMP-OPRGPLC-NEXT:         OMPSharedClause
      // DMP-OPRGPLC-OSIMP-SAME:     <implicit>
      // DMP-OPRGPLC-OSEXP-NOT:      <implicit>
      // DMP-OPRGPLC-NEXT:           DeclRefExpr {{.*}} 'j' 'int'
      // DMP-OPRGPLC-NEXT:           DeclRefExpr {{.*}} 'loopOnly' 'int'
      // DMP-OPRGPLC-NEXT:           DeclRefExpr {{.*}} 'save'
      // DMP-OPRGPLC-NEXT:           DeclRefExpr {{.*}} 'loopOnlyArr' 'int [1]'
      // DMP-OPRGPLC:              ForStmt
      // DMP-OSEQ-NEXT:          impl: ForStmt
      // DMP-OSEQPLC-NEXT:       impl: ForStmt
      //
      // PRT-A-NEXT:                {{^ *}}#pragma acc loop[[ACCC]]
      // PRT-AO-OSEQ-SAME:          {{^}} // discarded in OpenMP translation
      // PRT-AO-OSEQPLC-SAME:       {{^}} // discarded in OpenMP translation
      // PRT-A-SAME:                {{^$}}
      // PRT-AO-OPRG-OSIMP-NEXT:    {{^ *}}// #pragma omp [[OMPDP]]{{$}}
      // PRT-AO-OPRGPLC-OSIMP-NEXT: {{^ *}}// #pragma omp [[OMPDP]]{{$}}
      // PRT-AO-OPRG-OSEXP-NEXT:    {{^ *}}// #pragma omp [[OMPDP]] shared(j,loopOnly,save,loopOnlyArr){{$}}
      // PRT-AO-OPRGPLC-OSEXP-NEXT: {{^ *}}// #pragma omp [[OMPDP]] shared(j,loopOnly,save,loopOnlyArr){{$}}
      //
      // PRT-O-OPRG-OSIMP-NEXT:    {{^ *}}#pragma omp [[OMPDP]]{{$}}
      // PRT-O-OPRGPLC-OSIMP-NEXT: {{^ *}}#pragma omp [[OMPDP]]{{$}}
      // PRT-O-OPRG-OSEXP-NEXT:    {{^ *}}#pragma omp [[OMPDP]] shared(j,loopOnly,save,loopOnlyArr){{$}}
      // PRT-O-OPRGPLC-OSEXP-NEXT: {{^ *}}#pragma omp [[OMPDP]] shared(j,loopOnly,save,loopOnlyArr){{$}}
      // PRT-OA-NEXT:              {{^ *}}// #pragma acc loop[[ACCC]]
      // PRT-OA-OSEQ-SAME:         {{^}} // discarded in OpenMP translation
      // PRT-OA-OSEQPLC-SAME:      {{^}} // discarded in OpenMP translation
      // PRT-OA-SAME:              {{^$}}
      //
      // PRT-NEXT: for ({{.*}}) {
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
      } // PRT: }

      // Should see j value assigned inside loop except that, if
      // gang-partitioned, one of the three gangs won't run it.
      if (j == 0)
        save.declNestedLoopOld = true;
      else if (j == 2)
        save.declNestedLoopNew = true;
      else
        save.declNestedLoopErr = true;
    } // PRT: }

    // EXE-NEXT: save.readLoopOnlyOld=1
    printf("save.readLoopOnlyOld=%d\n", save.readLoopOnlyOld);
    // EXE-NOPART-NEXT:       save.readLoopOnlyNew=1
    // EXE-GPART-NEXT:        save.readLoopOnlyNew=0
    // EXE-PART-GREDUN-NEXT:  save.readLoopOnlyNew={{[01]}}
    printf("save.readLoopOnlyNew=%d\n", save.readLoopOnlyNew);
    // EXE: save.readLoopOnlyErr=0
    printf("save.readLoopOnlyErr=%d\n", save.readLoopOnlyErr);

    // EXE-NEXT: save.readLoopOnlyArrOld=1
    printf("save.readLoopOnlyArrOld=%d\n", save.readLoopOnlyArrOld);
    // EXE-NOPART-NEXT: save.readLoopOnlyArrNew=1
    // EXE-PART-NEXT: save.readLoopOnlyArrNew={{[01]}}
    printf("save.readLoopOnlyArrNew=%d\n", save.readLoopOnlyArrNew);
    // EXE-NEXT: save.readLoopOnlyArrErr=0
    printf("save.readLoopOnlyArrErr=%d\n", save.readLoopOnlyArrErr);

    // EXE-GREDUN-NEXT: save.declNestedLoopOld=0
    // EXE-GPART-NEXT:  save.declNestedLoopOld=1
    printf("save.declNestedLoopOld=%d\n", save.declNestedLoopOld);
    // EXE-NEXT: save.declNestedLoopNew=1
    printf("save.declNestedLoopNew=%d\n", save.declNestedLoopNew);
    // EXE-NEXT: save.declNestedLoopErr=0
    printf("save.declNestedLoopErr=%d\n", save.declNestedLoopErr);

    // EXE-NEXT: loopOnly=88
    printf("loopOnly=%d\n", loopOnly);
    // EXE-NEXT: loopOnlyArr[0]=77
    printf("loopOnlyArr[0]=%d\n", loopOnlyArr[0]);
  } // PRT: }

  // Repeat that but with acc parallel and acc loop combined.  Then, j cannot
  // be within acc parallel and outside acc loop, so we declare it outside acc
  // parallel and check it inside acc loop.

  // PRT-NEXT: {
  {
    // Accessed in acc loop but not otherwise in acc parallel.
    // PRT-NEXT: int loopOnly = 88;
    // PRT-NEXT: int loopOnlyArr[1] = {88};
    int loopOnly = 88;
    int loopOnlyArr[1] = {88};

    // PRT-NEXT: struct {
    struct {
      bool readLoopOnlyOld;
      bool readLoopOnlyNew;
      bool readLoopOnlyErr;

      bool readLoopOnlyArrOld;
      bool readLoopOnlyArrNew;
      bool readLoopOnlyArrErr;

      bool declNestedLoopOld;
      bool declNestedLoopNew;
      bool declNestedLoopErr;
    } save; // PRT: } save;

    // PRT-NEXT: int j = 0;
    int j = 0;

    // PRT-NEXT: memset
    memset(&save, 0, sizeof(save));

    // DMP:               ACCParallelLoopDirective
    // DMP-NEXT:            ACCNum_gangsClause
    // DMP-NEXT:              IntegerLiteral {{.*}} 'int' 3
    // DMP-ASEQ-NEXT:       ACCSeqClause
    // DMP-AIND-NEXT:       ACCIndependentClause
    // DMP-AAUTO-NEXT:      ACCAutoClause
    // DMP-ASEQ-NOT:        <implicit>
    // DMP-AIND-NOT:        <implicit>
    // DMP-AAUTO-NOT:       <implicit>
    // DMP-AG-NEXT:         ACCGangClause
    // DMP-AG-NOT:          <implicit>
    // DMP-AW-NEXT:         ACCWorkerClause
    // DMP-AV-NEXT:         ACCVectorClause
    // DMP-NEXT:            effect: ACCParallelDirective
    // DMP-NEXT:              ACCNum_gangsClause
    // DMP-NEXT:                IntegerLiteral {{.*}} 'int' 3
    // DMP-NEXT:              ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:                DeclRefExpr {{.*}} 'save'
    // DMP-NEXT:                DeclRefExpr {{.*}} 'loopOnlyArr' 'int [1]'
    // DMP-NEXT:              ACCFirstprivateClause {{.*}} <implicit>
    // DMP-NEXT:                DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:                DeclRefExpr {{.*}} 'loopOnly' 'int'
    // DMP-NEXT:              impl: OMPTargetTeamsDirective
    // DMP-NEXT:                OMPNum_teamsClause
    // DMP-NEXT:                  IntegerLiteral {{.*}} 'int' 3
    // DMP-NEXT:                OMPSharedClause
    // DMP-NOT:                   <implicit>
    // DMP-NEXT:                  DeclRefExpr {{.*}} 'save'
    // DMP-NEXT:                  DeclRefExpr {{.*}} 'loopOnlyArr' 'int [1]'
    // DMP-NEXT:                OMPFirstprivateClause
    // DMP-NOT:                   <implicit>
    // DMP-NEXT:                  DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:                  DeclRefExpr {{.*}} 'loopOnly' 'int'
    // DMP:                   ACCLoopDirective
    // DMP-ASEQ-NEXT:           ACCSeqClause
    // DMP-AIND-NEXT:           ACCIndependentClause
    // DMP-AAUTO-NEXT:          ACCAutoClause
    // DMP-ASEQ-NOT:            <implicit>
    // DMP-AIND-NOT:            <implicit>
    // DMP-AAUTO-NOT:           <implicit>
    // DMP-AG-NEXT:             ACCGangClause
    // DMP-AG-NOT:              <implicit>
    // DMP-AW-NEXT:             ACCWorkerClause
    // DMP-AV-NEXT:             ACCVectorClause
    // DMP-AIMP-NEXT:           ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:                ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:                  DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:                  DeclRefExpr {{.*}} 'loopOnly' 'int'
    // DMP-NEXT:                  DeclRefExpr {{.*}} 'save'
    // DMP-NEXT:                  DeclRefExpr {{.*}} 'loopOnlyArr' 'int [1]'
    // DMP-AGIMP-NEXT:          ACCGangClause {{.*}} <implicit>
    // DMP-OPRG-NEXT:           impl: [[OMPDD]]
    // DMP-OPRG-NEXT:             OMPSharedClause
    // DMP-OPRG-OSIMP-SAME:         <implicit>
    // DMP-OPRG-OSEXP-NOT:          <implicit>
    // DMP-OPRG-NEXT:               DeclRefExpr {{.*}} 'j' 'int'
    // DMP-OPRG-NEXT:               DeclRefExpr {{.*}} 'loopOnly' 'int'
    // DMP-OPRG-NEXT:               DeclRefExpr {{.*}} 'save'
    // DMP-OPRG-NEXT:               DeclRefExpr {{.*}} 'loopOnlyArr' 'int [1]'
    // DMP-OPRG:                  ForStmt
    // DMP-OPRGPLC-NEXT:        impl: [[OMPDD]]
    // DMP-ONT1-NEXT:             OMPNum_threadsClause
    // DMP-ONT1-NEXT:               IntegerLiteral {{.*}} 'int' 1
    // DMP-OPRGPLC-NEXT:          OMPSharedClause
    // DMP-OPRGPLC-OSIMP-SAME:      <implicit>
    // DMP-OPRGPLC-OSEXP-NOT:       <implicit>
    // DMP-OPRGPLC-NEXT:            DeclRefExpr {{.*}} 'j'
    // DMP-OPRGPLC-NEXT:            DeclRefExpr {{.*}} 'loopOnly' 'int'
    // DMP-OPRGPLC-NEXT:            DeclRefExpr {{.*}} 'save'
    // DMP-OPRGPLC-NEXT:            DeclRefExpr {{.*}} 'loopOnlyArr' 'int [1]'
    // DMP-OPRGPLC:               ForStmt
    // DMP-OSEQ-NEXT:           impl: ForStmt
    // DMP-OSEQPLC-NEXT:        impl: ForStmt
    //
    // PRT-A-NEXT:                {{^ *}}#pragma acc parallel loop num_gangs(3)[[ACCC]]{{$}}
    // PRT-AO-NEXT:               {{^ *}}// #pragma omp target teams num_teams(3) shared(save,loopOnlyArr) firstprivate(j,loopOnly){{$}}
    // PRT-AO-OPRG-OSIMP-NEXT:    {{^ *}}// #pragma omp [[OMPDP]]{{$}}
    // PRT-AO-OPRGPLC-OSIMP-NEXT: {{^ *}}// #pragma omp [[OMPDP]]{{$}}
    // PRT-AO-OPRG-OSEXP-NEXT:    {{^ *}}// #pragma omp [[OMPDP]] shared(j,loopOnly,save,loopOnlyArr){{$}}
    // PRT-AO-OPRGPLC-OSEXP-NEXT: {{^ *}}// #pragma omp [[OMPDP]] shared(j,loopOnly,save,loopOnlyArr){{$}}
    //
    // PRT-O-NEXT:               {{^ *}}#pragma omp target teams num_teams(3) shared(save,loopOnlyArr) firstprivate(j,loopOnly){{$}}
    // PRT-O-OPRG-OSIMP-NEXT:    {{^ *}}#pragma omp [[OMPDP]]{{$}}
    // PRT-O-OPRGPLC-OSIMP-NEXT: {{^ *}}#pragma omp [[OMPDP]]{{$}}
    // PRT-O-OPRG-OSEXP-NEXT:    {{^ *}}#pragma omp [[OMPDP]] shared(j,loopOnly,save,loopOnlyArr){{$}}
    // PRT-O-OPRGPLC-OSEXP-NEXT: {{^ *}}#pragma omp [[OMPDP]] shared(j,loopOnly,save,loopOnlyArr){{$}}
    // PRT-OA-NEXT:              {{^ *}}// #pragma acc parallel loop num_gangs(3)[[ACCC]]{{$}}
    //
    // PRT-NEXT: for ({{.*}}) {
    #pragma acc parallel loop num_gangs(3) ACCC
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

      // If gang-redundant and not otherwise partitioned, each gang should see
      // the j value assigned by the loop above.  If gang-partitioned but not
      // otherwise partitioned, each gang that is assigned an i iteration will
      // see the j value assigned by the loop above, and remaining gangs will
      // not.  If otherwise partitioned, there's a race.
      if (j == 0)
        save.declNestedLoopOld = true;
      else if (j == 2)
        save.declNestedLoopNew = true;
      else
        save.declNestedLoopErr = true;
    } // PRT: }

    // EXE-NEXT: save.readLoopOnlyOld=1
    printf("save.readLoopOnlyOld=%d\n", save.readLoopOnlyOld);
    // EXE-NOPART-NEXT:       save.readLoopOnlyNew=1
    // EXE-GPART-NEXT:        save.readLoopOnlyNew=0
    // EXE-PART-GREDUN-NEXT:  save.readLoopOnlyNew={{[01]}}
    printf("save.readLoopOnlyNew=%d\n", save.readLoopOnlyNew);
    // EXE: save.readLoopOnlyErr=0
    printf("save.readLoopOnlyErr=%d\n", save.readLoopOnlyErr);

    // EXE-NEXT: save.readLoopOnlyArrOld=1
    printf("save.readLoopOnlyArrOld=%d\n", save.readLoopOnlyArrOld);
    // EXE-NOPART-NEXT: save.readLoopOnlyArrNew=1
    // EXE-PART-NEXT: save.readLoopOnlyArrNew={{[01]}}
    printf("save.readLoopOnlyArrNew=%d\n", save.readLoopOnlyArrNew);
    // EXE-NEXT: save.readLoopOnlyArrErr=0
    printf("save.readLoopOnlyArrErr=%d\n", save.readLoopOnlyArrErr);

    // EXE-NOPART-GREDUN-NEXT: save.declNestedLoopOld=0
    printf("save.declNestedLoopOld=%d\n", save.declNestedLoopOld);
    // EXE-NOPART-GREDUN-NEXT: save.declNestedLoopNew=1
    printf("save.declNestedLoopNew=%d\n", save.declNestedLoopNew);
    // EXE-NOPART-GREDUN-NEXT: save.declNestedLoopErr=0
    printf("save.declNestedLoopErr=%d\n", save.declNestedLoopErr);

    // EXE:      loopOnly=88
    printf("loopOnly=%d\n", loopOnly);
    // EXE-NEXT: loopOnlyArr[0]=77
    printf("loopOnlyArr[0]=%d\n", loopOnlyArr[0]);
  } // PRT: }

  //--------------------------------------------------
  // Exercise assigned loop control variable, j, predetermined private when
  // the loop is partitioned, but implicitly shared when loop is sequential.
  //
  // Despite being a control variable on an inner loop, k shouldn't be
  // handled as the control variable for the enclosing acc loop and so
  // shouldn't be predetermined private.
  //
  // For collapse value greater than 1, this is checked in loop-collapse.c.
  //--------------------------------------------------

  // PRT: {
  {
    // PRT: struct {
    struct {
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
    } save; // PRT: } save;

    // PRT-NEXT: memset
    memset(&save, 0, sizeof(save));

    // DMP:      ACCParallelDirective
    // DMP-NEXT:   ACCNum_gangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 3
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'save'
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 3
    // DMP-NEXT:     OMPSharedClause
    // DMP-NOT:        <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'save'
    //
    // PRT-A:       {{^ *}}#pragma acc parallel num_gangs(3){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(3) shared(save){{$}}
    // PRT-O:       {{^ *}}#pragma omp target teams num_teams(3) shared(save){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(3){{$}}
    #pragma acc parallel num_gangs(3)
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      // PRT-NEXT: int j = 999;
      int j = 999;
      // PRT-NEXT: int k = 888;
      int k = 888;

      // DMP:                 ACCLoopDirective
      // DMP-ASEQ-NEXT:         ACCSeqClause
      // DMP-AIND-NEXT:         ACCIndependentClause
      // DMP-AAUTO-NEXT:        ACCAutoClause
      // DMP-ASEQ-NOT:          <implicit>
      // DMP-AIND-NOT:          <implicit>
      // DMP-AAUTO-NOT:         <implicit>
      // DMP-AG-NEXT:           ACCGangClause
      // DMP-AG-NOT:            <implicit>
      // DMP-AW-NEXT:           ACCWorkerClause
      // DMP-AV-NEXT:           ACCVectorClause
      // DMP-AIMP-NEXT:         ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:              ACCSharedClause {{.*}} <implicit>
      // DMP-ASLC-NEXT:           DeclRefExpr {{.*}} 'j' 'int'
      // DMP-NEXT:                DeclRefExpr {{.*}} 'k' 'int'
      // DMP-APLC-NEXT:         ACCPrivateClause {{.*}} <implicit>
      // DMP-APLC-NEXT:           DeclRefExpr {{.*}} 'j' 'int'
      // DMP-AGIMP-NEXT:        ACCGangClause {{.*}} <implicit>
      // DMP-OPRG-NEXT:         impl: [[OMPDD]]
      // DMP-OPRG-NEXT:           OMPSharedClause
      // DMP-OPRG-OSIMP-SAME:       <implicit>
      // DMP-OPRG-OSEXP-NOT:        <implicit>
      // DMP-OPRG-NEXT:             DeclRefExpr {{.*}} 'k' 'int'
      // DMP-OPRG-NEXT:           OMPPrivateClause
      // DMP-OPRG-NOT:              <implicit>
      // DMP-OPRG-NEXT:             DeclRefExpr {{.*}} 'j' 'int'
      // DMP-OPRG:                ForStmt
      // DMP-OPRGPLC-NEXT:      impl: CompoundStmt
      // DMP-OPRGPLC-NEXT:        DeclStmt
      // DMP-OPRGPLC-NEXT:          VarDecl {{.*}} j 'int'
      // DMP-OPRGPLC-NEXT:        [[OMPDD]]
      // DMP-ONT1-NEXT:             OMPNum_threadsClause
      // DMP-ONT1-NEXT:               IntegerLiteral {{.*}} 'int' 1
      // DMP-OPRGPLC:               ForStmt
      // DMP-OSEQ-NEXT:         impl: ForStmt
      // DMP-OSEQPLC-NEXT:      impl: CompoundStmt
      // DMP-OSEQPLC-NEXT:        DeclStmt
      // DMP-OSEQPLC-NEXT:          VarDecl {{.*}} j 'int'
      // DMP-OSEQPLC-NEXT:        ForStmt
      //
      // PRT-AO-OPRGPLC-NEXT:       // v----------ACC----------v
      // PRT-AO-OSEQPLC-NEXT:       // v----------ACC----------v
      // PRT-A-NEXT:                {{^ *}}#pragma acc loop[[ACCC]]
      // PRT-AO-OSEQ-SAME:          {{^}} // discarded in OpenMP translation
      // PRT-A-SAME:                {{^$}}
      // PRT-AO-OPRG-OSIMP-NEXT:    // #pragma omp [[OMPDP]] private(j){{$}}
      // PRT-AO-OPRG-OSEXP-NEXT:    // #pragma omp [[OMPDP]] shared(k) private(j){{$}}
      // PRT-A-NEXT:                for (j ={{.*}}) {
      // PRT-A-NEXT:                  printf
      // PRT-A-NEXT:                  for (k ={{.*}})
      // PRT-A-NEXT:                    ;
      // PRT-A-NEXT:                }
      // PRT-AO-OPRGPLC-NEXT:       // ---------ACC->OMP--------
      // PRT-AO-OPRGPLC-NEXT:       // {
      // PRT-AO-OPRGPLC-NEXT:       //   int j;
      // PRT-AO-OPRGPLC-OSIMP-NEXT: //   #pragma omp [[OMPDP]]{{$}}
      // PRT-AO-OPRGPLC-OSEXP-NEXT: //   #pragma omp [[OMPDP]] shared(k){{$}}
      // PRT-AO-OPRGPLC-NEXT:       //   for (j ={{.*}}) {
      // PRT-AO-OPRGPLC-NEXT:       //     printf
      // PRT-AO-OPRGPLC-NEXT:       //     for (k ={{.*}})
      // PRT-AO-OPRGPLC-NEXT:       //       ;
      // PRT-AO-OPRGPLC-NEXT:       //   }
      // PRT-AO-OPRGPLC-NEXT:       // }
      // PRT-AO-OPRGPLC-NEXT:       // ^----------OMP----------^
      // PRT-AO-OSEQPLC-NEXT:       // ---------ACC->OMP--------
      // PRT-AO-OSEQPLC-NEXT:       // {
      // PRT-AO-OSEQPLC-NEXT:       //   int j;
      // PRT-AO-OSEQPLC-NEXT:       //   for (j ={{.*}}) {
      // PRT-AO-OSEQPLC-NEXT:       //     printf
      // PRT-AO-OSEQPLC-NEXT:       //     for (k ={{.*}})
      // PRT-AO-OSEQPLC-NEXT:       //       ;
      // PRT-AO-OSEQPLC-NEXT:       //   }
      // PRT-AO-OSEQPLC-NEXT:       // }
      // PRT-AO-OSEQPLC-NEXT:       // ^----------OMP----------^
      //
      // PRT-OA-OPRGPLC-NEXT:       // v----------OMP----------v
      // PRT-OA-OSEQPLC-NEXT:       // v----------OMP----------v
      // PRT-O-OPRG-OSIMP-NEXT:     {{^ *}}#pragma omp [[OMPDP]] private(j){{$}}
      // PRT-O-OPRG-OSEXP-NEXT:     {{^ *}}#pragma omp [[OMPDP]] shared(k) private(j){{$}}
      // PRT-OA-OPRG-NEXT:          // #pragma acc loop[[ACCC]]{{$}}
      // PRT-OA-OSEQ-NEXT:          // #pragma acc loop[[ACCC]] // discarded in OpenMP translation{{$}}
      // PRT-O-OPRGPLC-NEXT:        {
      // PRT-O-OSEQPLC-NEXT:        {
      // PRT-O-OPRGPLC-NEXT:          int j;
      // PRT-O-OSEQPLC-NEXT:          int j;
      // PRT-O-OPRGPLC-OSIMP-NEXT:    {{^ *}}#pragma omp [[OMPDP]]{{$}}
      // PRT-O-OPRGPLC-OSEXP-NEXT:    {{^ *}}#pragma omp [[OMPDP]] shared(k){{$}}
      // PRT-O-NEXT:                  for (j ={{.*}}) {
      // PRT-O-NEXT:                    printf
      // PRT-O-NEXT:                    for (k ={{.*}})
      // PRT-O-NEXT:                      ;
      // PRT-O-NEXT:                  }
      // PRT-O-OPRGPLC-NEXT:        }
      // PRT-O-OSEQPLC-NEXT:        }
      // PRT-OA-OPRGPLC-NEXT:       // ---------OMP<-ACC--------
      // PRT-OA-OPRGPLC-NEXT:       // #pragma acc loop[[ACCC]]{{$}}
      // PRT-OA-OPRGPLC-NEXT:       // for (j ={{.*}}) {
      // PRT-OA-OPRGPLC-NEXT:       //   printf
      // PRT-OA-OPRGPLC-NEXT:       //   for (k ={{.*}})
      // PRT-OA-OPRGPLC-NEXT:       //     ;
      // PRT-OA-OPRGPLC-NEXT:       // }
      // PRT-OA-OPRGPLC-NEXT:       // ^----------ACC----------^
      // PRT-OA-OSEQPLC-NEXT:       // ---------OMP<-ACC--------
      // PRT-OA-OSEQPLC-NEXT:       // #pragma acc loop[[ACCC]]{{$}}
      // PRT-OA-OSEQPLC-NEXT:       // for (j ={{.*}}) {
      // PRT-OA-OSEQPLC-NEXT:       //   printf
      // PRT-OA-OSEQPLC-NEXT:       //   for (k ={{.*}})
      // PRT-OA-OSEQPLC-NEXT:       //     ;
      // PRT-OA-OSEQPLC-NEXT:       // }
      // PRT-OA-OSEQPLC-NEXT:       // ^----------ACC----------^
      //
      // PRT-NOACC-NEXT:            for (j ={{.*}}) {
      // PRT-NOACC-NEXT:              printf
      // PRT-NOACC-NEXT:              for (k ={{.*}})
      // PRT-NOACC-NEXT:                ;
      // PRT-NOACC-NEXT:            }
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
      if (j == 999)
        save.assignLoopOld = true;
      else if (j == 3)
        save.assignLoopNew = true;
      else
        save.assignLoopErr = true;
      // Should see k value assigned inside loop except that, if
      // gang-partitioned, one of the three gangs won't run it.
      if (k == 888)
        save.assignNestedLoopOld = true;
      else if (k == 2)
        save.assignNestedLoopNew = true;
      else
        save.assignNestedLoopErr = true;

      // PRT: j = 999;
      j = 999;

      // j is used here but it should have been dropped from the loop
      // control variable list, so it should now be implicitly shared in all
      // cases.
      //
      // DMP:                  ACCLoopDirective
      // DMP-ASEQ-NEXT:          ACCSeqClause
      // DMP-AIND-NEXT:          ACCIndependentClause
      // DMP-AAUTO-NEXT:         ACCAutoClause
      // DMP-ASEQ-NOT:           <implicit>
      // DMP-AIND-NOT:           <implicit>
      // DMP-AAUTO-NOT:          <implicit>
      // DMP-AG-NEXT:            ACCGangClause
      // DMP-AG-NOT:             <implicit>
      // DMP-AW-NEXT:            ACCWorkerClause
      // DMP-AV-NEXT:            ACCVectorClause
      // DMP-AIMP-NEXT:          ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:               ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:                 DeclRefExpr {{.*}} 'j' 'int'
      // DMP-NEXT:                 DeclRefExpr {{.*}} 'save'
      // DMP-AGIMP-NEXT:         ACCGangClause {{.*}} <implicit>
      // DMP-OPRG-NEXT:          impl: [[OMPDD]]
      // DMP-OPRG-NEXT:            OMPSharedClause
      // DMP-OPRG-OSIMP-SAME:        <implicit>
      // DMP-OPRG-OSEXP-NOT:         <implicit>
      // DMP-OPRG-NEXT:              DeclRefExpr {{.*}} 'j' 'int'
      // DMP-OPRG-NEXT:              DeclRefExpr {{.*}} 'save'
      // DMP-OPRG:                 ForStmt
      // DMP-OPRGPLC-NEXT:       impl: [[OMPDD]]
      // DMP-ONT1-NEXT:            OMPNum_threadsClause
      // DMP-ONT1-NEXT:              IntegerLiteral {{.*}} 'int' 1
      // DMP-OPRGPLC-NEXT:         OMPSharedClause
      // DMP-OPRGPLC-OSIMP-SAME:     <implicit>
      // DMP-OPRGPLC-OSEXP-NOT:      <implicit>
      // DMP-OPRGPLC-NEXT:           DeclRefExpr {{.*}} 'j' 'int'
      // DMP-OPRGPLC-NEXT:           DeclRefExpr {{.*}} 'save'
      // DMP-OPRGPLC:              ForStmt
      // DMP-OSEQ-NEXT:          impl: ForStmt
      // DMP-OSEQPLC-NEXT:       impl: ForStmt
      //
      // PRT-A-NEXT:                {{^ *}}#pragma acc loop[[ACCC]]
      // PRT-AO-OSEQ-SAME:          {{^}} // discarded in OpenMP translation
      // PRT-AO-OSEQPLC-SAME:       {{^}} // discarded in OpenMP translation
      // PRT-A-SAME:                {{^$}}
      // PRT-AO-OPRG-OSIMP-NEXT:    {{^ *}}// #pragma omp [[OMPDP]]{{$}}
      // PRT-AO-OPRGPLC-OSIMP-NEXT: {{^ *}}// #pragma omp [[OMPDP]]{{$}}
      // PRT-AO-OPRG-OSEXP-NEXT:    {{^ *}}// #pragma omp [[OMPDP]] shared(j,save){{$}}
      // PRT-AO-OPRGPLC-OSEXP-NEXT: {{^ *}}// #pragma omp [[OMPDP]] shared(j,save){{$}}
      //
      // PRT-O-OPRG-OSIMP-NEXT:     {{^ *}}#pragma omp [[OMPDP]]{{$}}
      // PRT-O-OPRGPLC-OSIMP-NEXT:  {{^ *}}#pragma omp [[OMPDP]]{{$}}
      // PRT-O-OPRG-OSEXP-NEXT:     {{^ *}}#pragma omp [[OMPDP]] shared(j,save){{$}}
      // PRT-O-OPRGPLC-OSEXP-NEXT:  {{^ *}}#pragma omp [[OMPDP]] shared(j,save){{$}}
      // PRT-OA-NEXT:               {{^ *}}// #pragma acc loop[[ACCC]]
      // PRT-OA-OSEQ-SAME:          {{^}} // discarded in OpenMP translation
      // PRT-OA-OSEQPLC-SAME:       {{^}} // discarded in OpenMP translation
      // PRT-OA-SAME:               {{^$}}
      //
      // PRT-NEXT: for ({{.*}}) {
      #pragma acc loop ACCC
      for (int i = 0; i < 2; ++i) {
        // If gang-redundant, every gang should see old j value at i = 0, new
        // j value at i = 1 if sequential, and either j value at i = 1 if
        // otherwise partitioned.  If gang-partitioned, each of two gangs should
        // see old j value at the gang's one assigned iteration, and one gang
        // shouldn't see anything.
        if (j == 999) // check old before new in case assign between
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
      } // PRT: }

      // Should see j value assigned inside loop except that, if
      // gang-partitioned, one of the three gangs won't run it.
      if (j == 999)
        save.writeInLaterLoopOld = true;
      else if (j == 9999)
        save.writeInLaterLoopNew = true;
      else
        save.writeInLaterLoopErr = true;
    } // PRT: }

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
    // EXE-NOPART-NEXT:       save.readInLaterLoopNew=1
    // EXE-GPART-NEXT:        save.readInLaterLoopNew=0
    // EXE-PART-GREDUN-NEXT:  save.readInLaterLoopNew={{[01]}}
    printf("save.readInLaterLoopNew=%d\n", save.readInLaterLoopNew);
    // EXE: save.readInLaterLoopErr=0
    printf("save.readInLaterLoopErr=%d\n", save.readInLaterLoopErr);

    // EXE-GREDUN-NEXT: save.writeInLaterLoopOld=0
    // EXE-GPART-NEXT:  save.writeInLaterLoopOld=1
    printf("save.writeInLaterLoopOld=%d\n", save.writeInLaterLoopOld);
    // EXE-NEXT: save.writeInLaterLoopNew=1
    printf("save.writeInLaterLoopNew=%d\n", save.writeInLaterLoopNew);
    // EXE-NEXT: save.writeInLaterLoopErr=0
    printf("save.writeInLaterLoopErr=%d\n", save.writeInLaterLoopErr);
  } // PRT: }

  // Repeat that but with acc parallel and acc loop combined.  Then, j and k
  // cannot be within acc parallel and outside acc loop, so we declare them
  // outside acc parallel and don't try to check their runtime visibility.
  // Dumps and prints should be sufficient at this point.

  // PRT: {
  {
    // PRT-NEXT: int j = 999;
    int j = 999;
    // PRT-NEXT: int k = 888;
    int k = 888;

    // DMP:               ACCParallelLoopDirective
    // DMP-ASEQ-NEXT:       ACCSeqClause
    // DMP-AIND-NEXT:       ACCIndependentClause
    // DMP-AAUTO-NEXT:      ACCAutoClause
    // DMP-ASEQ-NOT:        <implicit>
    // DMP-AIND-NOT:        <implicit>
    // DMP-AAUTO-NOT:       <implicit>
    // DMP-AG-NEXT:         ACCGangClause
    // DMP-AG-NOT:          <implicit>
    // DMP-AW-NEXT:         ACCWorkerClause
    // DMP-AV-NEXT:         ACCVectorClause
    // DMP-NEXT:            ACCNum_gangsClause
    // DMP-NEXT:              IntegerLiteral {{.*}} 'int' 3
    // DMP-NEXT:            effect: ACCParallelDirective
    // DMP-NEXT:              ACCNum_gangsClause
    // DMP-NEXT:                IntegerLiteral {{.*}} 'int' 3
    // DMP-NEXT:              ACCFirstprivateClause {{.*}} <implicit>
    // DMP-NEXT:                DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:                DeclRefExpr {{.*}} 'k' 'int'
    // DMP-NEXT:              impl: OMPTargetTeamsDirective
    // DMP-NEXT:                OMPNum_teamsClause
    // DMP-NEXT:                  IntegerLiteral {{.*}} 'int' 3
    // DMP-NEXT:                OMPFirstprivateClause
    // DMP-NOT:                   <implicit>
    // DMP-NEXT:                  DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:                  DeclRefExpr {{.*}} 'k' 'int'
    // DMP:                   ACCLoopDirective
    // DMP-ASEQ-NEXT:           ACCSeqClause
    // DMP-AIND-NEXT:           ACCIndependentClause
    // DMP-AAUTO-NEXT:          ACCAutoClause
    // DMP-ASEQ-NOT:            <implicit>
    // DMP-AIND-NOT:            <implicit>
    // DMP-AAUTO-NOT:           <implicit>
    // DMP-AG-NEXT:             ACCGangClause
    // DMP-AG-NOT:              <implicit>
    // DMP-AW-NEXT:             ACCWorkerClause
    // DMP-AV-NEXT:             ACCVectorClause
    // DMP-AIMP-NEXT:           ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:                ACCSharedClause {{.*}} <implicit>
    // DMP-ASLC-NEXT:             DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:                  DeclRefExpr {{.*}} 'k' 'int'
    // DMP-APLC-NEXT:           ACCPrivateClause {{.*}} <implicit>
    // DMP-APLC-NEXT:             DeclRefExpr {{.*}} 'j' 'int'
    // DMP-AGIMP-NEXT:          ACCGangClause {{.*}} <implicit>
    // DMP-OPRG-NEXT:           impl: [[OMPDD]]
    // DMP-OPRG-NEXT:             OMPSharedClause
    // DMP-OPRG-OSIMP-SAME:         <implicit>
    // DMP-OPRG-OSEXP-NOT:          <implicit>
    // DMP-OPRG-NEXT:               DeclRefExpr {{.*}} 'k' 'int'
    // DMP-OPRG-NEXT:             OMPPrivateClause
    // DMP-OPRG-NOT:                <implicit>
    // DMP-OPRG-NEXT:               DeclRefExpr {{.*}} 'j' 'int'
    // DMP-OPRG:                  ForStmt
    // DMP-OPRGPLC-NEXT:        impl: CompoundStmt
    // DMP-OPRGPLC-NEXT:          DeclStmt
    // DMP-OPRGPLC-NEXT:            VarDecl {{.*}} j 'int'
    // DMP-OPRGPLC-NEXT:          [[OMPDD]]
    // DMP-ONT1-NEXT:               OMPNum_threadsClause
    // DMP-ONT1-NEXT:                 IntegerLiteral {{.*}} 'int' 1
    // DMP-OPRGPLC:                 ForStmt
    // DMP-OSEQ-NEXT:           impl: ForStmt
    // DMP-OSEQPLC-NEXT:        impl: CompoundStmt
    // DMP-OSEQPLC-NEXT:          DeclStmt
    // DMP-OSEQPLC-NEXT:            VarDecl {{.*}} j 'int'
    // DMP-OSEQPLC-NEXT:          ForStmt
    //
    // PRT-AO-OPRGPLC-NEXT:       // v----------ACC----------v
    // PRT-AO-OSEQPLC-NEXT:       // v----------ACC----------v
    // PRT-A-NEXT:                {{^ *}}#pragma acc parallel loop[[ACCC]] num_gangs(3){{$}}
    // PRT-AO-OPRG-NEXT:          // #pragma omp target teams num_teams(3) firstprivate(j,k){{$}}
    // PRT-AO-OPRG-OSIMP-NEXT:    // #pragma omp [[OMPDP]] private(j){{$}}
    // PRT-AO-OPRG-OSEXP-NEXT:    // #pragma omp [[OMPDP]] shared(k) private(j){{$}}
    // PRT-AO-OSEQ-NEXT:          // #pragma omp target teams num_teams(3) firstprivate(j,k){{$}}
    // PRT-A-NEXT:                for (j ={{.*}}) {
    // PRT-A-NEXT:                  printf
    // PRT-A-NEXT:                  for (k ={{.*}})
    // PRT-A-NEXT:                    ;
    // PRT-A-NEXT:                }
    // PRT-AO-OPRGPLC-NEXT:       // ---------ACC->OMP--------
    // PRT-AO-OPRGPLC-NEXT:       // #pragma omp target teams num_teams(3) firstprivate(j,k){{$}}
    // PRT-AO-OPRGPLC-NEXT:       // {
    // PRT-AO-OPRGPLC-NEXT:       //   int j;
    // PRT-AO-OPRGPLC-OSIMP-NEXT: //   #pragma omp [[OMPDP]]{{$}}
    // PRT-AO-OPRGPLC-OSEXP-NEXT: //   #pragma omp [[OMPDP]] shared(k){{$}}
    // PRT-AO-OPRGPLC-NEXT:       //   for (j ={{.*}}) {
    // PRT-AO-OPRGPLC-NEXT:       //     printf
    // PRT-AO-OPRGPLC-NEXT:       //     for (k ={{.*}})
    // PRT-AO-OPRGPLC-NEXT:       //       ;
    // PRT-AO-OPRGPLC-NEXT:       //   }
    // PRT-AO-OPRGPLC-NEXT:       // }
    // PRT-AO-OPRGPLC-NEXT:       // ^----------OMP----------^
    // PRT-AO-OSEQPLC-NEXT:       // ---------ACC->OMP--------
    // PRT-AO-OSEQPLC-NEXT:       // #pragma omp target teams num_teams(3) firstprivate(j,k){{$}}
    // PRT-AO-OSEQPLC-NEXT:       // {
    // PRT-AO-OSEQPLC-NEXT:       //   int j;
    // PRT-AO-OSEQPLC-NEXT:       //   for (j ={{.*}}) {
    // PRT-AO-OSEQPLC-NEXT:       //     printf
    // PRT-AO-OSEQPLC-NEXT:       //     for (k ={{.*}})
    // PRT-AO-OSEQPLC-NEXT:       //       ;
    // PRT-AO-OSEQPLC-NEXT:       //   }
    // PRT-AO-OSEQPLC-NEXT:       // }
    // PRT-AO-OSEQPLC-NEXT:       // ^----------OMP----------^
    //
    // PRT-OA-OPRGPLC-NEXT:      // v----------OMP----------v
    // PRT-OA-OSEQPLC-NEXT:      // v----------OMP----------v
    // PRT-O-NEXT:               {{^ *}}#pragma omp target teams num_teams(3) firstprivate(j,k){{$}}
    // PRT-O-OPRG-OSIMP-NEXT:    {{^ *}}#pragma omp [[OMPDP]] private(j){{$}}
    // PRT-O-OPRG-OSEXP-NEXT:    {{^ *}}#pragma omp [[OMPDP]] shared(k) private(j){{$}}
    // PRT-OA-OPRG-NEXT:         // #pragma acc parallel loop[[ACCC]] num_gangs(3){{$}}
    // PRT-OA-OSEQ-NEXT:         // #pragma acc parallel loop[[ACCC]] num_gangs(3){{$}}
    // PRT-O-OPRGPLC-NEXT:       {
    // PRT-O-OSEQPLC-NEXT:       {
    // PRT-O-OPRGPLC-NEXT:         int j;
    // PRT-O-OSEQPLC-NEXT:         int j;
    // PRT-O-OPRGPLC-OSIMP-NEXT:   {{^ *}}#pragma omp [[OMPDP]]{{$}}
    // PRT-O-OPRGPLC-OSEXP-NEXT:   {{^ *}}#pragma omp [[OMPDP]] shared(k){{$}}
    // PRT-O-NEXT:                 for (j ={{.*}}) {
    // PRT-O-NEXT:                   printf
    // PRT-O-NEXT:                   for (k ={{.*}})
    // PRT-O-NEXT:                     ;
    // PRT-O-NEXT:                 }
    // PRT-O-OPRGPLC-NEXT:       }
    // PRT-O-OSEQPLC-NEXT:       }
    // PRT-OA-OPRGPLC-NEXT:      // ---------OMP<-ACC--------
    // PRT-OA-OPRGPLC-NEXT:      // #pragma acc parallel loop[[ACCC]] num_gangs(3){{$}}
    // PRT-OA-OPRGPLC-NEXT:      // for (j ={{.*}}) {
    // PRT-OA-OPRGPLC-NEXT:      //   printf
    // PRT-OA-OPRGPLC-NEXT:      //   for (k ={{.*}})
    // PRT-OA-OPRGPLC-NEXT:      //     ;
    // PRT-OA-OPRGPLC-NEXT:      // }
    // PRT-OA-OPRGPLC-NEXT:      // ^----------ACC----------^
    // PRT-OA-OSEQPLC-NEXT:      // ---------OMP<-ACC--------
    // PRT-OA-OSEQPLC-NEXT:      // #pragma acc parallel loop[[ACCC]] num_gangs(3){{$}}
    // PRT-OA-OSEQPLC-NEXT:      // for (j ={{.*}}) {
    // PRT-OA-OSEQPLC-NEXT:      //   printf
    // PRT-OA-OSEQPLC-NEXT:      //   for (k ={{.*}})
    // PRT-OA-OSEQPLC-NEXT:      //     ;
    // PRT-OA-OSEQPLC-NEXT:      // }
    // PRT-OA-OSEQPLC-NEXT:      // ^----------ACC----------^
    //
    // PRT-NOACC-NEXT:           for (j ={{.*}}) {
    // PRT-NOACC-NEXT:             printf
    // PRT-NOACC-NEXT:             for (k ={{.*}})
    // PRT-NOACC-NEXT:               ;
    // PRT-NOACC-NEXT:           }
    #pragma acc parallel loop ACCC num_gangs(3)
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
  } // PRT: }

  // Repeat that but explicitly declare the loop control variable firstprivate
  // to check that clause validation doesn't somehow think it conflicts with
  // the predetermined private on the effective acc loop.

  // PRT: {
  {
    // PRT-NEXT: int j = 999;
    int j = 999;
    // PRT-NEXT: int k = 888;
    int k = 888;

    // DMP:               ACCParallelLoopDirective
    // DMP-ASEQ-NEXT:       ACCSeqClause
    // DMP-AIND-NEXT:       ACCIndependentClause
    // DMP-AAUTO-NEXT:      ACCAutoClause
    // DMP-ASEQ-NOT:        <implicit>
    // DMP-AIND-NOT:        <implicit>
    // DMP-AAUTO-NOT:       <implicit>
    // DMP-AG-NEXT:         ACCGangClause
    // DMP-AG-NOT:          <implicit>
    // DMP-AW-NEXT:         ACCWorkerClause
    // DMP-AV-NEXT:         ACCVectorClause
    // DMP-NEXT:            ACCNum_gangsClause
    // DMP-NEXT:              IntegerLiteral {{.*}} 'int' 3
    // DMP-NEXT:            ACCFirstprivateClause
    // DMP-NOT:               <implicit>
    // DMP-NEXT:              DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:            effect: ACCParallelDirective
    // DMP-NEXT:              ACCNum_gangsClause
    // DMP-NEXT:                IntegerLiteral {{.*}} 'int' 3
    // DMP-NEXT:              ACCFirstprivateClause
    // DMP-NOT:                 <implicit>
    // DMP-NEXT:                DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:              ACCFirstprivateClause {{.*}} <implicit>
    // DMP-NEXT:                DeclRefExpr {{.*}} 'k' 'int'
    // DMP-NEXT:              impl: OMPTargetTeamsDirective
    // DMP-NEXT:                OMPNum_teamsClause
    // DMP-NEXT:                  IntegerLiteral {{.*}} 'int' 3
    // DMP-NEXT:                OMPFirstprivateClause
    // DMP-NOT:                   <implicit>
    // DMP-NEXT:                  DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:                OMPFirstprivateClause
    // DMP-NOT:                   <implicit>
    // DMP-NEXT:                  DeclRefExpr {{.*}} 'k' 'int'
    // DMP:                   ACCLoopDirective
    // DMP-ASEQ-NEXT:           ACCSeqClause
    // DMP-AIND-NEXT:           ACCIndependentClause
    // DMP-AAUTO-NEXT:          ACCAutoClause
    // DMP-ASEQ-NOT:            <implicit>
    // DMP-AIND-NOT:            <implicit>
    // DMP-AAUTO-NOT:           <implicit>
    // DMP-AG-NEXT:             ACCGangClause
    // DMP-AG-NOT:              <implicit>
    // DMP-AW-NEXT:             ACCWorkerClause
    // DMP-AV-NEXT:             ACCVectorClause
    // DMP-AIMP-NEXT:           ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:                ACCSharedClause {{.*}} <implicit>
    // DMP-ASLC-NEXT:             DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:                  DeclRefExpr {{.*}} 'k' 'int'
    // DMP-APLC-NEXT:           ACCPrivateClause {{.*}} <implicit>
    // DMP-APLC-NEXT:             DeclRefExpr {{.*}} 'j' 'int'
    // DMP-AGIMP-NEXT:          ACCGangClause {{.*}} <implicit>
    // DMP-OPRG-NEXT:           impl: [[OMPDD]]
    // DMP-OPRG-NEXT:             OMPSharedClause
    // DMP-OPRG-OSIMP-SAME:         <implicit>
    // DMP-OPRG-OSEXP-NOT:          <implicit>
    // DMP-OPRG-NEXT:               DeclRefExpr {{.*}} 'k' 'int'
    // DMP-OPRG-NEXT:             OMPPrivateClause
    // DMP-OPRG-NOT:                <implicit>
    // DMP-OPRG-NEXT:               DeclRefExpr {{.*}} 'j' 'int'
    // DMP-OPRG:                  ForStmt
    // DMP-OPRGPLC-NEXT:        impl: CompoundStmt
    // DMP-OPRGPLC-NEXT:          DeclStmt
    // DMP-OPRGPLC-NEXT:            VarDecl {{.*}} j 'int'
    // DMP-OPRGPLC-NEXT:          [[OMPDD]]
    // DMP-ONT1-NEXT:               OMPNum_threadsClause
    // DMP-ONT1-NEXT:                 IntegerLiteral {{.*}} 'int' 1
    // DMP-OPRGPLC:                 ForStmt
    // DMP-OSEQ-NEXT:           impl: ForStmt
    // DMP-OSEQPLC-NEXT:        impl: CompoundStmt
    // DMP-OSEQPLC-NEXT:          DeclStmt
    // DMP-OSEQPLC-NEXT:            VarDecl {{.*}} j 'int'
    // DMP-OSEQPLC-NEXT:            ForStmt
    //
    // PRT-AO-OPRGPLC-NEXT:       // v----------ACC----------v
    // PRT-AO-OSEQPLC-NEXT:       // v----------ACC----------v
    // PRT-A-NEXT:                {{^ *}}#pragma acc parallel loop[[ACCC]] num_gangs(3) firstprivate(j){{$}}
    // PRT-AO-OPRG-NEXT:          // #pragma omp target teams num_teams(3) firstprivate(j) firstprivate(k){{$}}
    // PRT-AO-OSEQ-NEXT:          // #pragma omp target teams num_teams(3) firstprivate(j) firstprivate(k){{$}}
    // PRT-AO-OPRG-OSIMP-NEXT:    // #pragma omp [[OMPDP]] private(j){{$}}
    // PRT-AO-OPRG-OSEXP-NEXT:    // #pragma omp [[OMPDP]] shared(k) private(j){{$}}
    // PRT-A-NEXT:                for (j ={{.*}}) {
    // PRT-A-NEXT:                  printf
    // PRT-A-NEXT:                  for (k ={{.*}})
    // PRT-A-NEXT:                    ;
    // PRT-A-NEXT:                }
    // PRT-AO-OPRGPLC-NEXT:       // ---------ACC->OMP--------
    // PRT-AO-OPRGPLC-NEXT:       // #pragma omp target teams num_teams(3) firstprivate(j) firstprivate(k){{$}}
    // PRT-AO-OPRGPLC-NEXT:       // {
    // PRT-AO-OPRGPLC-NEXT:       //   int j;
    // PRT-AO-OPRGPLC-OSIMP-NEXT: //   #pragma omp [[OMPDP]]{{$}}
    // PRT-AO-OPRGPLC-OSEXP-NEXT: //   #pragma omp [[OMPDP]] shared(k){{$}}
    // PRT-AO-OPRGPLC-NEXT:       //   for (j ={{.*}}) {
    // PRT-AO-OPRGPLC-NEXT:       //     printf
    // PRT-AO-OPRGPLC-NEXT:       //     for (k ={{.*}})
    // PRT-AO-OPRGPLC-NEXT:       //       ;
    // PRT-AO-OPRGPLC-NEXT:       //   }
    // PRT-AO-OPRGPLC-NEXT:       // }
    // PRT-AO-OPRGPLC-NEXT:       // ^----------OMP----------^
    // PRT-AO-OSEQPLC-NEXT:       // ---------ACC->OMP--------
    // PRT-AO-OSEQPLC-NEXT:       // #pragma omp target teams num_teams(3) firstprivate(j) firstprivate(k){{$}}
    // PRT-AO-OSEQPLC-NEXT:       // {
    // PRT-AO-OSEQPLC-NEXT:       //   int j;
    // PRT-AO-OSEQPLC-NEXT:       //   for (j ={{.*}}) {
    // PRT-AO-OSEQPLC-NEXT:       //     printf
    // PRT-AO-OSEQPLC-NEXT:       //     for (k ={{.*}})
    // PRT-AO-OSEQPLC-NEXT:       //       ;
    // PRT-AO-OSEQPLC-NEXT:       //   }
    // PRT-AO-OSEQPLC-NEXT:       // }
    // PRT-AO-OSEQPLC-NEXT:       // ^----------OMP----------^
    //
    // PRT-OA-OPRGPLC-NEXT:      // v----------OMP----------v
    // PRT-OA-OSEQPLC-NEXT:      // v----------OMP----------v
    // PRT-O-NEXT:               {{^ *}}#pragma omp target teams num_teams(3) firstprivate(j) firstprivate(k){{$}}
    // PRT-O-OPRG-OSIMP-NEXT:    {{^ *}}#pragma omp [[OMPDP]] private(j){{$}}
    // PRT-O-OPRG-OSEXP-NEXT:    {{^ *}}#pragma omp [[OMPDP]] shared(k) private(j){{$}}
    // PRT-OA-OPRG-NEXT:         // #pragma acc parallel loop[[ACCC]] num_gangs(3) firstprivate(j){{$}}
    // PRT-OA-OSEQ-NEXT:         // #pragma acc parallel loop[[ACCC]] num_gangs(3) firstprivate(j){{$}}
    // PRT-O-OPRGPLC-NEXT:       {
    // PRT-O-OSEQPLC-NEXT:       {
    // PRT-O-OPRGPLC-NEXT:         int j;
    // PRT-O-OSEQPLC-NEXT:         int j;
    // PRT-O-OPRGPLC-OSIMP-NEXT:   {{^ *}}#pragma omp [[OMPDP]]{{$}}
    // PRT-O-OPRGPLC-OSEXP-NEXT:   {{^ *}}#pragma omp [[OMPDP]] shared(k){{$}}
    // PRT-O-NEXT:                 for (j ={{.*}}) {
    // PRT-O-NEXT:                   printf
    // PRT-O-NEXT:                   for (k ={{.*}})
    // PRT-O-NEXT:                     ;
    // PRT-O-NEXT:                 }
    // PRT-O-OPRGPLC-NEXT:       }
    // PRT-O-OSEQPLC-NEXT:       }
    // PRT-OA-OPRGPLC-NEXT:      // ---------OMP<-ACC--------
    // PRT-OA-OPRGPLC-NEXT:      // #pragma acc parallel loop[[ACCC]] num_gangs(3) firstprivate(j){{$}}
    // PRT-OA-OPRGPLC-NEXT:      // for (j ={{.*}}) {
    // PRT-OA-OPRGPLC-NEXT:      //   printf
    // PRT-OA-OPRGPLC-NEXT:      //   for (k ={{.*}})
    // PRT-OA-OPRGPLC-NEXT:      //     ;
    // PRT-OA-OPRGPLC-NEXT:      // }
    // PRT-OA-OPRGPLC-NEXT:      // ^----------ACC----------^
    // PRT-OA-OSEQPLC-NEXT:      // ---------OMP<-ACC--------
    // PRT-OA-OSEQPLC-NEXT:      // #pragma acc parallel loop[[ACCC]] num_gangs(3) firstprivate(j){{$}}
    // PRT-OA-OSEQPLC-NEXT:      // for (j ={{.*}}) {
    // PRT-OA-OSEQPLC-NEXT:      //   printf
    // PRT-OA-OSEQPLC-NEXT:      //   for (k ={{.*}})
    // PRT-OA-OSEQPLC-NEXT:      //     ;
    // PRT-OA-OSEQPLC-NEXT:      // }
    // PRT-OA-OSEQPLC-NEXT:      // ^----------ACC----------^
    //
    // PRT-NOACC-NEXT:           for (j ={{.*}}) {
    // PRT-NOACC-NEXT:             printf
    // PRT-NOACC-NEXT:             for (k ={{.*}})
    // PRT-NOACC-NEXT:               ;
    // PRT-NOACC-NEXT:           }
    #pragma acc parallel loop ACCC num_gangs(3) firstprivate(j)
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
  } // PRT: }

  //--------------------------------------------------
  // Check an assigned loop control variable that has only a tentative
  // definition.  Inserting a local definition to make that loop control
  // variable private for a vector-partitioned loop used to fail an assert
  // because VarDecl::getDefinition returned nullptr for the tentative
  // definition.
  //--------------------------------------------------

  // PRT: {
  {
    // DMP:      ACCParallelDirective
    // DMP-NEXT:   ACCNum_gangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 3
    // DMP-NEXT:   ACCFirstprivateClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'tentativeDef' 'int'
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 3
    // DMP-NEXT:     OMPFirstprivateClause
    // DMP-NOT:        <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'tentativeDef' 'int'
    //
    // PRT-A:       {{^ *}}#pragma acc parallel num_gangs(3){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(3) firstprivate(tentativeDef){{$}}
    // PRT-O:       {{^ *}}#pragma omp target teams num_teams(3) firstprivate(tentativeDef){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(3){{$}}
    #pragma acc parallel num_gangs(3)
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      // DMP:                 ACCLoopDirective
      // DMP-ASEQ-NEXT:         ACCSeqClause
      // DMP-AIND-NEXT:         ACCIndependentClause
      // DMP-AAUTO-NEXT:        ACCAutoClause
      // DMP-ASEQ-NOT:          <implicit>
      // DMP-AIND-NOT:          <implicit>
      // DMP-AAUTO-NOT:         <implicit>
      // DMP-AG-NEXT:           ACCGangClause
      // DMP-AG-NOT:            <implicit>
      // DMP-AW-NEXT:           ACCWorkerClause
      // DMP-AV-NEXT:           ACCVectorClause
      // DMP-AIMP-NEXT:         ACCIndependentClause {{.*}} <implicit>
      // DMP-ASLC-NEXT:         ACCSharedClause {{.*}} <implicit>
      // DMP-APLC-NEXT:         ACCPrivateClause {{.*}} <implicit>
      // DMP-NEXT:                DeclRefExpr {{.*}} 'tentativeDef' 'int'
      // DMP-AGIMP-NEXT:        ACCGangClause {{.*}} <implicit>
      // DMP-OPRG-NEXT:         impl: [[OMPDD]]
      // DMP-OPRG-NEXT:           OMPPrivateClause
      // DMP-OPRG-NOT:              <implicit>
      // DMP-OPRG-NEXT:             DeclRefExpr {{.*}} 'tentativeDef' 'int'
      // DMP-OPRG:                ForStmt
      // DMP-OPRGPLC-NEXT:      impl: CompoundStmt
      // DMP-OPRGPLC-NEXT:        DeclStmt
      // DMP-OPRGPLC-NEXT:          VarDecl {{.*}} tentativeDef 'int'
      // DMP-OPRGPLC-NEXT:        [[OMPDD]]
      // DMP-ONT1-NEXT:             OMPNum_threadsClause
      // DMP-ONT1-NEXT:               IntegerLiteral {{.*}} 'int' 1
      // DMP-OPRGPLC:               ForStmt
      // DMP-OSEQ-NEXT:         impl: ForStmt
      // DMP-OSEQPLC-NEXT:      impl: CompoundStmt
      // DMP-OSEQPLC-NEXT:        DeclStmt
      // DMP-OSEQPLC-NEXT:          VarDecl {{.*}} tentativeDef 'int'
      // DMP-OSEQPLC-NEXT:          ForStmt
      //
      // PRT-AO-OPRGPLC-NEXT: // v----------ACC----------v
      // PRT-AO-OSEQPLC-NEXT: // v----------ACC----------v
      // PRT-A-NEXT:          {{^ *}}#pragma acc loop[[ACCC]]
      // PRT-AO-OSEQ-SAME:    {{^}} // discarded in OpenMP translation
      // PRT-A-SAME:          {{^$}}
      // PRT-AO-OPRG-NEXT:    // #pragma omp [[OMPDP]] private(tentativeDef){{$}}
      // PRT-A-NEXT:          for (tentativeDef ={{.*}}) {
      // PRT-A-NEXT:            printf
      // PRT-A-NEXT:          }
      // PRT-AO-OPRGPLC-NEXT: // ---------ACC->OMP--------
      // PRT-AO-OPRGPLC-NEXT: // {
      // PRT-AO-OPRGPLC-NEXT: //   int tentativeDef;
      // PRT-AO-OPRGPLC-NEXT: //   #pragma omp [[OMPDP]]{{$}}
      // PRT-AO-OPRGPLC-NEXT: //   for (tentativeDef ={{.*}}) {
      // PRT-AO-OPRGPLC-NEXT: //     printf
      // PRT-AO-OPRGPLC-NEXT: //   }
      // PRT-AO-OPRGPLC-NEXT: // }
      // PRT-AO-OPRGPLC-NEXT: // ^----------OMP----------^
      // PRT-AO-OSEQPLC-NEXT: // ---------ACC->OMP--------
      // PRT-AO-OSEQPLC-NEXT: // {
      // PRT-AO-OSEQPLC-NEXT: //   int tentativeDef;
      // PRT-AO-OSEQPLC-NEXT: //   for (tentativeDef ={{.*}}) {
      // PRT-AO-OSEQPLC-NEXT: //     printf
      // PRT-AO-OSEQPLC-NEXT: //   }
      // PRT-AO-OSEQPLC-NEXT: // }
      // PRT-AO-OSEQPLC-NEXT: // ^----------OMP----------^
      //
      // PRT-OA-OPRGPLC-NEXT: // v----------OMP----------v
      // PRT-OA-OSEQPLC-NEXT: // v----------OMP----------v
      // PRT-O-OPRG-NEXT:     {{^ *}}#pragma omp [[OMPDP]] private(tentativeDef){{$}}
      // PRT-OA-OPRG-NEXT:    // #pragma acc loop[[ACCC]]{{$}}
      // PRT-OA-OSEQ-NEXT:    // #pragma acc loop[[ACCC]] // discarded in OpenMP translation{{$}}
      // PRT-O-OPRGPLC-NEXT:  {
      // PRT-O-OSEQPLC-NEXT:  {
      // PRT-O-OPRGPLC-NEXT:    int tentativeDef;
      // PRT-O-OSEQPLC-NEXT:    int tentativeDef;
      // PRT-O-OPRGPLC-NEXT:    {{^ *}}#pragma omp [[OMPDP]]{{$}}
      // PRT-O-NEXT:            for (tentativeDef ={{.*}}) {
      // PRT-O-NEXT:              printf
      // PRT-O-NEXT:            }
      // PRT-O-OPRGPLC-NEXT:  }
      // PRT-O-OSEQPLC-NEXT:  }
      // PRT-OA-OPRGPLC-NEXT: // ---------OMP<-ACC--------
      // PRT-OA-OPRGPLC-NEXT: // #pragma acc loop[[ACCC]]{{$}}
      // PRT-OA-OPRGPLC-NEXT: // for (tentativeDef ={{.*}}) {
      // PRT-OA-OPRGPLC-NEXT: //   printf
      // PRT-OA-OPRGPLC-NEXT: // }
      // PRT-OA-OPRGPLC-NEXT: // ^----------ACC----------^
      // PRT-OA-OSEQPLC-NEXT: // ---------OMP<-ACC--------
      // PRT-OA-OSEQPLC-NEXT: // #pragma acc loop[[ACCC]]{{$}}
      // PRT-OA-OSEQPLC-NEXT: // for (tentativeDef ={{.*}}) {
      // PRT-OA-OSEQPLC-NEXT: //   printf
      // PRT-OA-OSEQPLC-NEXT: // }
      // PRT-OA-OSEQPLC-NEXT: // ^----------ACC----------^
      //
      // PRT-NOACC-NEXT: for (tentativeDef ={{.*}}) {
      // PRT-NOACC-NEXT:   printf
      // PRT-NOACC-NEXT: }
      #pragma acc loop ACCC
      for (tentativeDef = 1; tentativeDef < 3; ++tentativeDef) {
        // EXE-NEXT:        hello world
        // EXE-NEXT:        hello world
        // EXE-GREDUN-NEXT: hello world
        // EXE-GREDUN-NEXT: hello world
        // EXE-GREDUN-NEXT: hello world
        // EXE-GREDUN-NEXT: hello world
        printf("hello world\n");
      }
    } // PRT: }
  } // PRT: }

  // Repeat that but with acc parallel and acc loop combined.

  // PRT: {
  {
    // DMP:               ACCParallelLoopDirective
    // DMP-NEXT:            ACCNum_gangsClause
    // DMP-NEXT:              IntegerLiteral {{.*}} 'int' 3
    // DMP-ASEQ-NEXT:       ACCSeqClause
    // DMP-AIND-NEXT:       ACCIndependentClause
    // DMP-AAUTO-NEXT:      ACCAutoClause
    // DMP-ASEQ-NOT:        <implicit>
    // DMP-AIND-NOT:        <implicit>
    // DMP-AAUTO-NOT:       <implicit>
    // DMP-AG-NEXT:         ACCGangClause
    // DMP-AG-NOT:          <implicit>
    // DMP-AW-NEXT:         ACCWorkerClause
    // DMP-AV-NEXT:         ACCVectorClause
    // DMP-NEXT:            effect: ACCParallelDirective
    // DMP-NEXT:              ACCNum_gangsClause
    // DMP-NEXT:                IntegerLiteral {{.*}} 'int' 3
    // DMP-NEXT:              ACCFirstprivateClause {{.*}} <implicit>
    // DMP-NEXT:                DeclRefExpr {{.*}} 'tentativeDef' 'int'
    // DMP-NEXT:              impl: OMPTargetTeamsDirective
    // DMP-NEXT:                OMPNum_teamsClause
    // DMP-NEXT:                  IntegerLiteral {{.*}} 'int' 3
    // DMP-NEXT:                OMPFirstprivateClause
    // DMP-NOT:                   <implicit>
    // DMP-NEXT:                  DeclRefExpr {{.*}} 'tentativeDef' 'int'
    // DMP:                   ACCLoopDirective
    // DMP-ASEQ-NEXT:           ACCSeqClause
    // DMP-AIND-NEXT:           ACCIndependentClause
    // DMP-AAUTO-NEXT:          ACCAutoClause
    // DMP-ASEQ-NOT:            <implicit>
    // DMP-AIND-NOT:            <implicit>
    // DMP-AAUTO-NOT:           <implicit>
    // DMP-AG-NEXT:             ACCGangClause
    // DMP-AG-NOT:              <implicit>
    // DMP-AW-NEXT:             ACCWorkerClause
    // DMP-AV-NEXT:             ACCVectorClause
    // DMP-AIMP-NEXT:           ACCIndependentClause {{.*}} <implicit>
    // DMP-ASLC-NEXT:           ACCSharedClause {{.*}} <implicit>
    // DMP-APLC-NEXT:           ACCPrivateClause {{.*}} <implicit>
    // DMP-NEXT:                  DeclRefExpr {{.*}} 'tentativeDef' 'int'
    // DMP-AGIMP-NEXT:          ACCGangClause {{.*}} <implicit>
    // DMP-OPRG-NEXT:           impl: [[OMPDD]]
    // DMP-OPRG-NEXT:             OMPPrivateClause
    // DMP-OPRG-NOT:                <implicit>
    // DMP-OPRG-NEXT:               DeclRefExpr {{.*}} 'tentativeDef' 'int'
    // DMP-OPRG:                  ForStmt
    // DMP-OPRGPLC-NEXT:        impl: CompoundStmt
    // DMP-OPRGPLC-NEXT:          DeclStmt
    // DMP-OPRGPLC-NEXT:            VarDecl {{.*}} tentativeDef 'int'
    // DMP-OPRGPLC-NEXT:          [[OMPDD]]
    // DMP-ONT1-NEXT:               OMPNum_threadsClause
    // DMP-ONT1-NEXT:                 IntegerLiteral {{.*}} 'int' 1
    // DMP-OPRGPLC:                 ForStmt
    // DMP-OSEQ-NEXT:           impl: ForStmt
    // DMP-OSEQPLC-NEXT:        impl: CompoundStmt
    // DMP-OSEQPLC-NEXT:          DeclStmt
    // DMP-OSEQPLC-NEXT:            VarDecl {{.*}} tentativeDef 'int'
    // DMP-OSEQPLC-NEXT:          ForStmt
    //
    //
    // PRT-AO-OPRGPLC-NEXT: // v----------ACC----------v
    // PRT-AO-OSEQPLC-NEXT: // v----------ACC----------v
    // PRT-A:               {{^ *}}#pragma acc parallel loop num_gangs(3)[[ACCC]]{{$}}
    // PRT-AO-OPRG-NEXT:    {{^ *}}// #pragma omp target teams num_teams(3) firstprivate(tentativeDef){{$}}
    // PRT-AO-OSEQ-NEXT:    {{^ *}}// #pragma omp target teams num_teams(3) firstprivate(tentativeDef){{$}}
    // PRT-AO-OPRG-NEXT:    {{^ *}}// #pragma omp [[OMPDP]] private(tentativeDef){{$}}
    // PRT-A-NEXT:          for (tentativeDef ={{.*}}) {
    // PRT-A-NEXT:            printf
    // PRT-A-NEXT:          }
    // PRT-AO-OPRGPLC-NEXT: // ---------ACC->OMP--------
    // PRT-AO-OPRGPLC-NEXT: // #pragma omp target teams num_teams(3) firstprivate(tentativeDef){{$}}
    // PRT-AO-OPRGPLC-NEXT: // {
    // PRT-AO-OPRGPLC-NEXT: //   int tentativeDef;
    // PRT-AO-OPRGPLC-NEXT: //   #pragma omp [[OMPDP]]{{$}}
    // PRT-AO-OPRGPLC-NEXT: //   for (tentativeDef ={{.*}}) {
    // PRT-AO-OPRGPLC-NEXT: //     printf
    // PRT-AO-OPRGPLC-NEXT: //   }
    // PRT-AO-OPRGPLC-NEXT: // }
    // PRT-AO-OPRGPLC-NEXT: // ^----------OMP----------^
    // PRT-AO-OSEQPLC-NEXT: // ---------ACC->OMP--------
    // PRT-AO-OSEQPLC-NEXT: // #pragma omp target teams num_teams(3) firstprivate(tentativeDef){{$}}
    // PRT-AO-OSEQPLC-NEXT: // {
    // PRT-AO-OSEQPLC-NEXT: //   int tentativeDef;
    // PRT-AO-OSEQPLC-NEXT: //   for (tentativeDef ={{.*}}) {
    // PRT-AO-OSEQPLC-NEXT: //     printf
    // PRT-AO-OSEQPLC-NEXT: //   }
    // PRT-AO-OSEQPLC-NEXT: // }
    // PRT-AO-OSEQPLC-NEXT: // ^----------OMP----------^
    //
    // PRT-OA-OPRGPLC-NEXT: // v----------OMP----------v
    // PRT-OA-OSEQPLC-NEXT: // v----------OMP----------v
    // PRT-O-NEXT:          {{^ *}}#pragma omp target teams num_teams(3) firstprivate(tentativeDef){{$}}
    // PRT-O-OPRG-NEXT:     {{^ *}}#pragma omp [[OMPDP]] private(tentativeDef){{$}}
    // PRT-OA-OPRG-NEXT:    {{^ *}}// #pragma acc parallel loop num_gangs(3)[[ACCC]]{{$}}
    // PRT-OA-OSEQ-NEXT:    {{^ *}}// #pragma acc parallel loop num_gangs(3)[[ACCC]]{{$}}
    // PRT-O-OPRGPLC-NEXT:  {
    // PRT-O-OSEQPLC-NEXT:  {
    // PRT-O-OPRGPLC-NEXT:    int tentativeDef;
    // PRT-O-OSEQPLC-NEXT:    int tentativeDef;
    // PRT-O-OPRGPLC-NEXT:    {{^ *}}#pragma omp [[OMPDP]]{{$}}
    // PRT-O-NEXT:            for (tentativeDef ={{.*}}) {
    // PRT-O-NEXT:              printf
    // PRT-O-NEXT:            }
    // PRT-O-OPRGPLC-NEXT:  }
    // PRT-O-OSEQPLC-NEXT:  }
    // PRT-OA-OPRGPLC-NEXT: // ---------OMP<-ACC--------
    // PRT-OA-OPRGPLC-NEXT: // #pragma acc parallel loop num_gangs(3)[[ACCC]]{{$}}
    // PRT-OA-OPRGPLC-NEXT: // for (tentativeDef ={{.*}}) {
    // PRT-OA-OPRGPLC-NEXT: //   printf
    // PRT-OA-OPRGPLC-NEXT: // }
    // PRT-OA-OPRGPLC-NEXT: // ^----------ACC----------^
    // PRT-OA-OSEQPLC-NEXT: // ---------OMP<-ACC--------
    // PRT-OA-OSEQPLC-NEXT: // #pragma acc parallel loop num_gangs(3)[[ACCC]]{{$}}
    // PRT-OA-OSEQPLC-NEXT: // for (tentativeDef ={{.*}}) {
    // PRT-OA-OSEQPLC-NEXT: //   printf
    // PRT-OA-OSEQPLC-NEXT: // }
    // PRT-OA-OSEQPLC-NEXT: // ^----------ACC----------^
    //
    // PRT-NOACC-NEXT: for (tentativeDef ={{.*}}) {
    // PRT-NOACC-NEXT:   printf
    // PRT-NOACC-NEXT: }
    #pragma acc parallel loop num_gangs(3) ACCC
    for (tentativeDef = 1; tentativeDef < 3; ++tentativeDef) {
      // EXE-NEXT:        hello world
      // EXE-NEXT:        hello world
      // EXE-GREDUN-NEXT: hello world
      // EXE-GREDUN-NEXT: hello world
      // EXE-GREDUN-NEXT: hello world
      // EXE-GREDUN-NEXT: hello world
      printf("hello world\n");
    }
  } // PRT: }

  //--------------------------------------------------
  // A sequential loop should be able to affect its number of iterations when
  // modifying its control variable in its body.  A partitioned loop just uses
  // the number of iterations specified in the for init, cond, and incr.
  //--------------------------------------------------

  // DMP:      ACCParallelDirective
  // DMP-NEXT:   impl: OMPTargetTeamsDirective
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc parallel{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams{{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp target teams{{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel{{$}}
  #pragma acc parallel
  // DMP:               ACCLoopDirective
  // DMP-ASEQ-NEXT:       ACCSeqClause
  // DMP-AIND-NEXT:       ACCIndependentClause
  // DMP-AAUTO-NEXT:      ACCAutoClause
  // DMP-ASEQ-NOT:        <implicit>
  // DMP-AIND-NOT:        <implicit>
  // DMP-AAUTO-NOT:       <implicit>
  // DMP-AG-NEXT:         ACCGangClause
  // DMP-AG-NOT:          <implicit>
  // DMP-AW-NEXT:         ACCWorkerClause
  // DMP-AV-NEXT:         ACCVectorClause
  // DMP-AIMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
  // DMP-AGIMP-NEXT:      ACCGangClause {{.*}} <implicit>
  // DMP-OPRG-NEXT:       impl: [[OMPDD]]
  // DMP-OPRG:              ForStmt
  // DMP-OPRGPLC-NEXT:    impl: [[OMPDD]]
  // DMP-ONT1-NEXT:         OMPNum_threadsClause
  // DMP-ONT1-NEXT:           IntegerLiteral {{.*}} 'int' 1
  // DMP-OPRGPLC:           ForStmt
  // DMP-OSEQ-NEXT:       impl: ForStmt
  // DMP-OSEQPLC-NEXT:    impl: ForStmt
  //
  // PRT-A-NEXT:          {{^ *}}#pragma acc loop[[ACCC]]
  // PRT-AO-OSEQ-SAME:    {{^}} // discarded in OpenMP translation
  // PRT-AO-OSEQPLC-SAME: {{^}} // discarded in OpenMP translation
  // PRT-A-SAME:          {{^$}}
  // PRT-AO-OPRG-NEXT:    {{^ *}}// #pragma omp [[OMPDP]]{{$}}
  // PRT-AO-OPRGPLC-NEXT: {{^ *}}// #pragma omp [[OMPDP]]{{$}}
  //
  // PRT-O-OPRG-NEXT:     {{^ *}}#pragma omp [[OMPDP]]{{$}}
  // PRT-O-OPRGPLC-NEXT:  {{^ *}}#pragma omp [[OMPDP]]{{$}}
  // PRT-OA-NEXT:         {{^ *}}// #pragma acc loop[[ACCC]]
  // PRT-OA-OSEQ-SAME:    {{^}} // discarded in OpenMP translation
  // PRT-OA-OSEQPLC-SAME: {{^}} // discarded in OpenMP translation
  // PRT-OA-SAME:         {{^$}}
  //
  // PRT-NEXT: for ({{.*}}) {
  #pragma acc loop ACCC
  for (int i = 0; i < 2; ++i) {
    // EXE-NEXT:      just once
    // EXE-PART-NEXT: just once
    printf("just once\n");
    i = 2;
  } // PRT: }

  // Repeat that but with acc parallel and acc loop combined.

  // DMP:               ACCParallelLoopDirective
  // DMP-ASEQ-NEXT:       ACCSeqClause
  // DMP-AIND-NEXT:       ACCIndependentClause
  // DMP-AAUTO-NEXT:      ACCAutoClause
  // DMP-ASEQ-NOT:        <implicit>
  // DMP-AIND-NOT:        <implicit>
  // DMP-AAUTO-NOT:       <implicit>
  // DMP-AG-NEXT:         ACCGangClause
  // DMP-AG-NOT:          <implicit>
  // DMP-AW-NEXT:         ACCWorkerClause
  // DMP-AV-NEXT:         ACCVectorClause
  // DMP-NEXT:            effect: ACCParallelDirective
  // DMP-NEXT:              impl: OMPTargetTeamsDirective
  // DMP:                   ACCLoopDirective
  // DMP-ASEQ-NEXT:           ACCSeqClause
  // DMP-AIND-NEXT:           ACCIndependentClause
  // DMP-AAUTO-NEXT:          ACCAutoClause
  // DMP-ASEQ-NOT:            <implicit>
  // DMP-AIND-NOT:            <implicit>
  // DMP-AAUTO-NOT:           <implicit>
  // DMP-AG-NEXT:             ACCGangClause
  // DMP-AG-NOT:              <implicit>
  // DMP-AW-NEXT:             ACCWorkerClause
  // DMP-AV-NEXT:             ACCVectorClause
  // DMP-AIMP-NEXT:           ACCIndependentClause {{.*}} <implicit>
  // DMP-AGIMP-NEXT:          ACCGangClause {{.*}} <implicit>
  // DMP-OPRG-NEXT:           impl: [[OMPDD]]
  // DMP-OPRG:                  ForStmt
  // DMP-OPRGPLC-NEXT:        impl: [[OMPDD]]
  // DMP-ONT1-NEXT:             OMPNum_threadsClause
  // DMP-ONT1-NEXT:               IntegerLiteral {{.*}} 'int' 1
  // DMP-OPRGPLC:               ForStmt
  // DMP-OSEQ-NEXT:           impl: ForStmt
  // DMP-OSEQPLC-NEXT:        impl: ForStmt
  //
  // PRT-A-NEXT:          {{^ *}}#pragma acc parallel loop[[ACCC]]{{$}}
  // PRT-AO-NEXT:         {{^ *}}// #pragma omp target teams{{$}}
  // PRT-AO-OPRG-NEXT:    {{^ *}}// #pragma omp [[OMPDP]]{{$}}
  // PRT-AO-OPRGPLC-NEXT: {{^ *}}// #pragma omp [[OMPDP]]{{$}}
  //
  // PRT-O-NEXT:         {{^ *}}#pragma omp target teams{{$}}
  // PRT-O-OPRG-NEXT:    {{^ *}}#pragma omp [[OMPDP]]{{$}}
  // PRT-O-OPRGPLC-NEXT: {{^ *}}#pragma omp [[OMPDP]]{{$}}
  // PRT-OA-NEXT:        {{^ *}}// #pragma acc parallel loop[[ACCC]]{{$}}
  //
  // PRT-NEXT: for ({{.*}}) {
  #pragma acc parallel loop ACCC
  for (int i = 0; i < 2; ++i) {
    // EXE-NEXT:      just once
    // EXE-PART-NEXT: just once
    printf("just once\n");
    i = 2;
  } // PRT: }

  //--------------------------------------------------
  // Make sure that an init!=0 or inc!=1 work correctly.
  // The latter is specifically relevant to simd directives, which depend on
  // OpenMP's predetermined linear attribute with a step that is the loop inc.
  //--------------------------------------------------

  // DMP:      ACCParallelDirective
  // DMP-NEXT:   impl: OMPTargetTeamsDirective
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc parallel{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams{{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp target teams{{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel{{$}}
  #pragma acc parallel
  // PRT-NEXT: {
  {
    // PRT-NEXT: int j;
    int j;
    // DMP:               ACCLoopDirective
    // DMP-ASEQ-NEXT:       ACCSeqClause
    // DMP-AIND-NEXT:       ACCIndependentClause
    // DMP-AAUTO-NEXT:      ACCAutoClause
    // DMP-ASEQ-NOT:        <implicit>
    // DMP-AIND-NOT:        <implicit>
    // DMP-AAUTO-NOT:       <implicit>
    // DMP-AG-NEXT:         ACCGangClause
    // DMP-AG-NOT:          <implicit>
    // DMP-AW-NEXT:         ACCWorkerClause
    // DMP-AV-NEXT:         ACCVectorClause
    // DMP-AIMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
    // DMP-ASLC-NEXT:       ACCSharedClause {{.*}} <implicit>
    // DMP-ASLC-NEXT:         DeclRefExpr {{.*}} 'j' 'int'
    // DMP-APLC-NEXT:       ACCPrivateClause {{.*}} <implicit>
    // DMP-APLC-NEXT:         DeclRefExpr {{.*}} 'j' 'int'
    // DMP-AGIMP-NEXT:      ACCGangClause {{.*}} <implicit>
    // DMP-OPRG-NEXT:       impl: [[OMPDD]]
    // DMP-OPRG-NEXT:         OMPPrivateClause
    // DMP-OPRG-NOT:            <implicit>
    // DMP-OPRG-NEXT:           DeclRefExpr {{.*}} 'j' 'int'
    // DMP-OPRG:              ForStmt
    // DMP-OPRGPLC-NEXT:    impl: CompoundStmt
    // DMP-OPRGPLC-NEXT:      DeclStmt
    // DMP-OPRGPLC-NEXT:        VarDecl {{.*}} j 'int'
    // DMP-OPRGPLC-NEXT:      [[OMPDD]]
    // DMP-ONT1-NEXT:           OMPNum_threadsClause
    // DMP-ONT1-NEXT:             IntegerLiteral {{.*}} 'int' 1
    // DMP-OPRGPLC:             ForStmt
    // DMP-OSEQ-NEXT:       impl: ForStmt
    // DMP-OSEQPLC-NEXT:    impl: CompoundStmt
    // DMP-OSEQPLC-NEXT:      DeclStmt
    // DMP-OSEQPLC-NEXT:        VarDecl {{.*}} j 'int'
    // DMP-OSEQPLC-NEXT:      ForStmt
    //
    // PRT-AO-OPRGPLC-NEXT: // v----------ACC----------v
    // PRT-AO-OSEQPLC-NEXT: // v----------ACC----------v
    // PRT-A-NEXT:          {{^ *}}#pragma acc loop[[ACCC]]
    // PRT-AO-OSEQ-SAME:    {{^}} // discarded in OpenMP translation
    // PRT-A-SAME:          {{^$}}
    // PRT-AO-OPRG-NEXT:    {{^ *}}// #pragma omp [[OMPDP]] private(j){{$}}
    // PRT-A-NEXT:          for (j ={{.*}}) {
    // PRT-A-NEXT:            printf
    // PRT-A-NEXT:          }
    // PRT-AO-OPRGPLC-NEXT: // ---------ACC->OMP--------
    // PRT-AO-OPRGPLC-NEXT: // {
    // PRT-AO-OPRGPLC-NEXT: //   int j;
    // PRT-AO-OPRGPLC-NEXT: //   #pragma omp [[OMPDP]]{{$}}
    // PRT-AO-OPRGPLC-NEXT: //   for (j ={{.*}}) {
    // PRT-AO-OPRGPLC-NEXT: //     printf
    // PRT-AO-OPRGPLC-NEXT: //   }
    // PRT-AO-OPRGPLC-NEXT: // }
    // PRT-AO-OPRGPLC-NEXT: // ^----------OMP----------^
    // PRT-AO-OSEQPLC-NEXT: // ---------ACC->OMP--------
    // PRT-AO-OSEQPLC-NEXT: // {
    // PRT-AO-OSEQPLC-NEXT: //   int j;
    // PRT-AO-OSEQPLC-NEXT: //   for (j ={{.*}}) {
    // PRT-AO-OSEQPLC-NEXT: //     printf
    // PRT-AO-OSEQPLC-NEXT: //   }
    // PRT-AO-OSEQPLC-NEXT: // }
    // PRT-AO-OSEQPLC-NEXT: // ^----------OMP----------^
    //
    // PRT-OA-OPRGPLC-NEXT: // v----------OMP----------v
    // PRT-OA-OSEQPLC-NEXT: // v----------OMP----------v
    // PRT-O-OPRG-NEXT:     {{^ *}}#pragma omp [[OMPDP]] private(j){{$}}
    // PRT-OA-OPRG-NEXT:    {{^ *}}// #pragma acc loop[[ACCC]]{{$}}
    // PRT-OA-OSEQ-NEXT:    {{^ *}}// #pragma acc loop[[ACCC]] // discarded in OpenMP translation{{$}}
    // PRT-O-OPRGPLC-NEXT:  {
    // PRT-O-OSEQPLC-NEXT:  {
    // PRT-O-OPRGPLC-NEXT:    int j;
    // PRT-O-OSEQPLC-NEXT:    int j;
    // PRT-O-OPRGPLC-NEXT:    {{^ *}}#pragma omp [[OMPDP]]{{$}}
    // PRT-O-NEXT:            for (j ={{.*}}) {
    // PRT-O-NEXT:              printf
    // PRT-O-NEXT:            }
    // PRT-O-OPRGPLC-NEXT:  }
    // PRT-O-OSEQPLC-NEXT:  }
    // PRT-OA-OPRGPLC-NEXT: // ---------OMP<-ACC--------
    // PRT-OA-OPRGPLC-NEXT: // #pragma acc loop[[ACCC]]{{$}}
    // PRT-OA-OPRGPLC-NEXT: // for (j ={{.*}}) {
    // PRT-OA-OPRGPLC-NEXT: //   printf
    // PRT-OA-OPRGPLC-NEXT: // }
    // PRT-OA-OPRGPLC-NEXT: // ^----------ACC----------^
    // PRT-OA-OSEQPLC-NEXT: // ---------OMP<-ACC--------
    // PRT-OA-OSEQPLC-NEXT: // #pragma acc loop[[ACCC]]{{$}}
    // PRT-OA-OSEQPLC-NEXT: // for (j ={{.*}}) {
    // PRT-OA-OSEQPLC-NEXT: //   printf
    // PRT-OA-OSEQPLC-NEXT: // }
    // PRT-OA-OSEQPLC-NEXT: // ^----------ACC----------^
    //
    // PRT-NOACC-NEXT: for (j ={{.*}}) {
    // PRT-NOACC-NEXT:   printf
    // PRT-NOACC-NEXT: }
    #pragma acc loop ACCC
    for (j = 7; j > -2; j -= 2) {
      // EXE-DAG: 7
      // EXE-DAG: 5
      // EXE-DAG: 3
      // EXE-DAG: {{^1}}
      // EXE-DAG: -1
      printf("%d\n", j);
    }
  } // PRT-NEXT: }

  // Repeat that but with acc parallel and acc loop combined.

  // PRT-NEXT: {
  {
    // PRT-NEXT: int j;
    int j;
    // DMP:              ACCParallelLoopDirective
    // DMP-ASEQ-NEXT:      ACCSeqClause
    // DMP-AIND-NEXT:      ACCIndependentClause
    // DMP-AAUTO-NEXT:     ACCAutoClause
    // DMP-ASEQ-NOT:       <implicit>
    // DMP-AIND-NOT:       <implicit>
    // DMP-AAUTO-NOT:      <implicit>
    // DMP-AG-NEXT:        ACCGangClause
    // DMP-AG-NOT:         <implicit>
    // DMP-AW-NEXT:        ACCWorkerClause
    // DMP-AV-NEXT:        ACCVectorClause
    // DMP-NEXT:           effect: ACCParallelDirective
    // DMP-NEXT:             ACCFirstprivateClause {{.*}} <implicit>
    // DMP-NEXT:                DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:             impl: OMPTargetTeamsDirective
    // DMP-NEXT:               OMPFirstprivateClause
    // DMP-NOT:                   <implicit>
    // DMP-NEXT:                  DeclRefExpr {{.*}} 'j' 'int'
    // DMP:                  ACCLoopDirective
    // DMP-ASEQ-NEXT:          ACCSeqClause
    // DMP-AIND-NEXT:          ACCIndependentClause
    // DMP-AAUTO-NEXT:         ACCAutoClause
    // DMP-ASEQ-NOT:           <implicit>
    // DMP-AIND-NOT:           <implicit>
    // DMP-AAUTO-NOT:          <implicit>
    // DMP-AG-NEXT:            ACCGangClause
    // DMP-AG-NOT:             <implicit>
    // DMP-AW-NEXT:            ACCWorkerClause
    // DMP-AV-NEXT:            ACCVectorClause
    // DMP-AIMP-NEXT:          ACCIndependentClause {{.*}} <implicit>
    // DMP-ASLC-NEXT:          ACCSharedClause {{.*}} <implicit>
    // DMP-ASLC-NEXT:            DeclRefExpr {{.*}} 'j' 'int'
    // DMP-APLC-NEXT:          ACCPrivateClause {{.*}} <implicit>
    // DMP-APLC-NEXT:            DeclRefExpr {{.*}} 'j' 'int'
    // DMP-AGIMP-NEXT:         ACCGangClause {{.*}} <implicit>
    // DMP-OPRG-NEXT:          impl: [[OMPDD]]
    // DMP-OPRG-NEXT:            OMPPrivateClause
    // DMP-OPRG-NOT:               <implicit>
    // DMP-OPRG-NEXT:              DeclRefExpr {{.*}} 'j' 'int'
    // DMP-OPRG:                 ForStmt
    // DMP-OPRGPLC-NEXT:       impl: CompoundStmt
    // DMP-OPRGPLC-NEXT:         DeclStmt
    // DMP-OPRGPLC-NEXT:           VarDecl {{.*}} j 'int'
    // DMP-OPRGPLC-NEXT:         [[OMPDD]]
    // DMP-ONT1-NEXT:              OMPNum_threadsClause
    // DMP-ONT1-NEXT:                IntegerLiteral {{.*}} 'int' 1
    // DMP-OPRGPLC:                ForStmt
    // DMP-OSEQ-NEXT:          impl: ForStmt
    // DMP-OSEQPLC-NEXT:       impl: CompoundStmt
    // DMP-OSEQPLC-NEXT:         DeclStmt
    // DMP-OSEQPLC-NEXT:           VarDecl {{.*}} j 'int'
    // DMP-OSEQPLC-NEXT:         ForStmt
    //
    // PRT-AO-OPRGPLC-NEXT: // v----------ACC----------v
    // PRT-AO-OSEQPLC-NEXT: // v----------ACC----------v
    // PRT-A-NEXT:          {{^ *}}#pragma acc parallel loop[[ACCC]]{{$}}
    // PRT-AO-OPRG-NEXT:    // #pragma omp target teams firstprivate(j){{$}}
    // PRT-AO-OSEQ-NEXT:    // #pragma omp target teams firstprivate(j){{$}}
    // PRT-AO-OPRG-NEXT:    // #pragma omp [[OMPDP]] private(j){{$}}
    // PRT-A-NEXT:          for (j ={{.*}}) {
    // PRT-A-NEXT:            printf
    // PRT-A-NEXT:          }
    // PRT-AO-OPRGPLC-NEXT: // ---------ACC->OMP--------
    // PRT-AO-OPRGPLC-NEXT: // #pragma omp target teams firstprivate(j){{$}}
    // PRT-AO-OPRGPLC-NEXT: // {
    // PRT-AO-OPRGPLC-NEXT: //   int j;
    // PRT-AO-OPRGPLC-NEXT: //   #pragma omp [[OMPDP]]{{$}}
    // PRT-AO-OPRGPLC-NEXT: //   for (j ={{.*}}) {
    // PRT-AO-OPRGPLC-NEXT: //     printf
    // PRT-AO-OPRGPLC-NEXT: //   }
    // PRT-AO-OPRGPLC-NEXT: // }
    // PRT-AO-OPRGPLC-NEXT: // ^----------OMP----------^
    // PRT-AO-OSEQPLC-NEXT: // ---------ACC->OMP--------
    // PRT-AO-OSEQPLC-NEXT: // #pragma omp target teams firstprivate(j){{$}}
    // PRT-AO-OSEQPLC-NEXT: // {
    // PRT-AO-OSEQPLC-NEXT: //   int j;
    // PRT-AO-OSEQPLC-NEXT: //   for (j ={{.*}}) {
    // PRT-AO-OSEQPLC-NEXT: //     printf
    // PRT-AO-OSEQPLC-NEXT: //   }
    // PRT-AO-OSEQPLC-NEXT: // }
    // PRT-AO-OSEQPLC-NEXT: // ^----------OMP----------^
    //
    // PRT-OA-OPRGPLC-NEXT: // v----------OMP----------v
    // PRT-OA-OSEQPLC-NEXT: // v----------OMP----------v
    // PRT-O-NEXT:          {{^ *}}#pragma omp target teams firstprivate(j){{$}}
    // PRT-O-OPRG-NEXT:     {{^ *}}#pragma omp [[OMPDP]] private(j){{$}}
    // PRT-OA-OPRG-NEXT:    {{^ *}}// #pragma acc parallel loop[[ACCC]]{{$}}
    // PRT-OA-OSEQ-NEXT:    {{^ *}}// #pragma acc parallel loop[[ACCC]]{{$}}
    // PRT-O-OPRGPLC-NEXT:  {
    // PRT-O-OSEQPLC-NEXT:  {
    // PRT-O-OPRGPLC-NEXT:    int j;
    // PRT-O-OSEQPLC-NEXT:    int j;
    // PRT-O-OPRGPLC-NEXT:    {{^ *}}#pragma omp [[OMPDP]]{{$}}
    // PRT-O-NEXT:            for (j ={{.*}}) {
    // PRT-O-NEXT:              printf
    // PRT-O-NEXT:            }
    // PRT-O-OPRGPLC-NEXT:  }
    // PRT-O-OSEQPLC-NEXT:  }
    // PRT-OA-OPRGPLC-NEXT: // ---------OMP<-ACC--------
    // PRT-OA-OPRGPLC-NEXT: // #pragma acc parallel loop[[ACCC]]{{$}}
    // PRT-OA-OPRGPLC-NEXT: // for (j ={{.*}}) {
    // PRT-OA-OPRGPLC-NEXT: //   printf
    // PRT-OA-OPRGPLC-NEXT: // }
    // PRT-OA-OPRGPLC-NEXT: // ^----------ACC----------^
    // PRT-OA-OSEQPLC-NEXT: // ---------OMP<-ACC--------
    // PRT-OA-OSEQPLC-NEXT: // #pragma acc parallel loop[[ACCC]]{{$}}
    // PRT-OA-OSEQPLC-NEXT: // for (j ={{.*}}) {
    // PRT-OA-OSEQPLC-NEXT: //   printf
    // PRT-OA-OSEQPLC-NEXT: // }
    // PRT-OA-OSEQPLC-NEXT: // ^----------ACC----------^
    //
    // PRT-NOACC-NEXT: for (j ={{.*}}) {
    // PRT-NOACC-NEXT:   printf
    // PRT-NOACC-NEXT: }
    #pragma acc parallel loop ACCC
    for (j = 7; j > -2; j -= 2) {
      // EXE-DAG: 7
      // EXE-DAG: 5
      // EXE-DAG: 3
      // EXE-DAG: {{^1}}
      // EXE-DAG: -1
      printf("%d\n", j);
    }
  } // PRT-NEXT: }

  //--------------------------------------------------
  // Exercise a case for which clang used to misbehave due to clang's
  // implementation of the pre-OpenMP-3.1 predetermined shared attribute for
  // variables with const-qualified type having no mutable members.
  //--------------------------------------------------

  // PRT-NEXT: {
  {
    // PRT-NEXT: const int x = 55;
    const int x = 55;
    // DMP:      ACCParallelDirective
    // DMP-NEXT:   ACCFirstprivateClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'x' 'const int'
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPFirstprivateClause
    // DMP-NOT:        <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'x' 'const int'
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel{{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams firstprivate(x){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams firstprivate(x){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel{{$}}
    #pragma acc parallel
    // PRT-NEXT: {
    {
      // DMP:                    ACCLoopDirective
      // DMP-ASEQ-NEXT:            ACCSeqClause
      // DMP-AIND-NEXT:            ACCIndependentClause
      // DMP-AAUTO-NEXT:           ACCAutoClause
      // DMP-ASEQ-NOT:             <implicit>
      // DMP-AIND-NOT:             <implicit>
      // DMP-AAUTO-NOT:            <implicit>
      // DMP-AG-NEXT:              ACCGangClause
      // DMP-AG-NOT:               <implicit>
      // DMP-AW-NEXT:              ACCWorkerClause
      // DMP-AV-NEXT:              ACCVectorClause
      // DMP-AIMP-NEXT:            ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:                 ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:                   DeclRefExpr {{.*}} 'x' 'const int'
      // DMP-AGIMP-NEXT:           ACCGangClause {{.*}} <implicit>
      // DMP-OPRG-NEXT:            impl: [[OMPDD]]
      // DMP-OPRG-NEXT:              OMPSharedClause
      // DMP-OPRG-OSIMP-SAME:          <implicit>
      // DMP-OPRG-OSEXP-NOT:           <implicit>
      // DMP-OPRG-NEXT:                DeclRefExpr {{.*}} 'x' 'const int'
      // DMP-OPRG:                   ForStmt
      // DMP-OPRGPLC-NEXT:         impl: [[OMPDD]]
      // DMP-ONT1-NEXT:              OMPNum_threadsClause
      // DMP-ONT1-NEXT:                IntegerLiteral {{.*}} 'int' 1
      // DMP-OPRGPLC-NEXT:           OMPSharedClause
      // DMP-OPRGPLC-OSIMP-SAME:       <implicit>
      // DMP-OPRGPLC-OSEXP-NOT:        <implicit>
      // DMP-OPRGPLC-NEXT:             DeclRefExpr {{.*}} 'x' 'const int'
      // DMP-OPRGPLC:                ForStmt
      // DMP-OSEQ-NEXT:            impl: ForStmt
      // DMP-OSEQPLC-NEXT:         impl: ForStmt
      //
      // PRT-A-NEXT:                {{^ *}}#pragma acc loop[[ACCC]]
      // PRT-AO-OSEQ-SAME:          {{^}} // discarded in OpenMP translation
      // PRT-AO-OSEQPLC-SAME:       {{^}} // discarded in OpenMP translation
      // PRT-A-SAME:                {{^$}}
      // PRT-AO-OPRG-OSIMP-NEXT:    {{^ *}}// #pragma omp [[OMPDP]]{{$}}
      // PRT-AO-OPRG-OSEXP-NEXT:    {{^ *}}// #pragma omp [[OMPDP]] shared(x){{$}}
      // PRT-AO-OPRGPLC-OSIMP-NEXT: {{^ *}}// #pragma omp [[OMPDP]]{{$}}
      // PRT-AO-OPRGPLC-OSEXP-NEXT: {{^ *}}// #pragma omp [[OMPDP]] shared(x){{$}}
      //
      // PRT-O-OPRG-OSIMP-NEXT:     {{^ *}}#pragma omp [[OMPDP]]{{$}}
      // PRT-O-OPRG-OSEXP-NEXT:     {{^ *}}#pragma omp [[OMPDP]] shared(x){{$}}
      // PRT-O-OPRGPLC-OSIMP-NEXT:  {{^ *}}#pragma omp [[OMPDP]]{{$}}
      // PRT-O-OPRGPLC-OSEXP-NEXT:  {{^ *}}#pragma omp [[OMPDP]] shared(x){{$}}
      // PRT-OA-OPRG-NEXT:          {{^ *}}// #pragma acc loop[[ACCC]]{{$}}
      // PRT-OA-OPRGPLC-NEXT:       {{^ *}}// #pragma acc loop[[ACCC]]{{$}}
      // PRT-OA-OSEQ-NEXT:          {{^ *}}// #pragma acc loop[[ACCC]] // discarded in OpenMP translation{{$}}
      // PRT-OA-OSEQPLC-NEXT:       {{^ *}}// #pragma acc loop[[ACCC]] // discarded in OpenMP translation{{$}}
      //
      // PRT-NEXT: for (int i ={{.*}}) {
      // PRT-NEXT:   printf
      // PRT-NEXT: }
      #pragma acc loop ACCC
      for (int i = 0; i < 2; ++i) {
        // EXE-DAG: 0: 55
        // EXE-DAG: 1: 55
        printf("%d: %d\n", i, x);
      }
    } // PRT-NEXT: }
  } // PRT-NEXT: }

  // PRT-NEXT: return 0;
  return 0;
} // PRT-NEXT: }

// EXE-NOT: {{.}}
