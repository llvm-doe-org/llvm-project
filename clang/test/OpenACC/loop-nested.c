// Abbreviations:
//   A     = OpenACC
//   AO    = commented OpenMP is printed after OpenACC
//   O     = OpenMP
//   OA    = commented OpenACC is printed after OpenMP
//   AOX   = either AO or OA
//   AIMP  = OpenACC implicit independent
//   ASEQ  = OpenACC seq clause
//   AG    = OpenACC gang clause
//   AW    = OpenACC worker clause
//   AV    = OpenACC vector clause
//   OPRG  = OpenACC pragma translated to OpenMP pragma
//   OSEQ  = OpenACC loop seq discarded in translation to OpenMP
//   OSIMP = OpenMP shared implicit
//   OSEXP = OpenMP shared explicit
//   ONT1  = OpenMP num_threads(1)
//
//   accc  = OpenACC clauses
//   itrs  = loop iteration count
//   ompdd = OpenMP directive dump
//   ompdp = OpenMP directive print
//   ompdk = OpenMP directive kind (OPRG or OSEQ)
//   ompsk = OpenMP shared kind (OSIMP or OSEXP)
//   dmp   = additional FileCheck prefixes for dump
//   exe   = additional FileCheck prefixes for execution
//
// RUN: %data loop-clauses {
// RUN:   (accc0=gang                                accc1=worker                                  accc2=vector
// RUN:    itrs0=2                                   itrs1=2                                       itrs2=2
// RUN:    ompdd0=OMPDistributeDirective             ompdd1=OMPParallelForDirective                ompdd2=OMPSimdDirective
// RUN:    ompdp0=distribute                         ompdp1='parallel for'                         ompdp2='simd'
// RUN:    ompdk0=OPRG ompsk0=OSIMP                  ompdk1=OPRG ompsk1=OSEXP                      ompdk2=OPRG ompsk2=OSIMP
// RUN:    dmp0=DMP0-AIMP,DMP0-AG                    dmp1=DMP1-AIMP,DMP1-AW                        dmp2=DMP2-AIMP,DMP2-AV
// RUN:    exe=EXE222)
// RUN:   (accc0='gang worker'                       accc1=seq                                     accc2=vector
// RUN:    itrs0=4                                   itrs1=2                                       itrs2=2
// RUN:    ompdd0=OMPDistributeParallelForDirective  ompdd1=                                       ompdd2=OMPSimdDirective
// RUN:    ompdp0='distribute parallel for'          ompdp1=                                       ompdp2='simd'
// RUN:    ompdk0=OPRG ompsk0=OSEXP                  ompdk1=OSEQ ompsk1=OSIMP                      ompdk2=OPRG ompsk2=OSIMP
// RUN:    dmp0=DMP0-AIMP,DMP0-AG,DMP0-AW            dmp1=DMP1-ASEQ                                dmp2=DMP2-AIMP,DMP2-AV
// RUN:    exe=EXE,EXE422)
// RUN:   (accc0=gang                                accc1=seq                                     accc2='worker vector'
// RUN:    itrs0=2                                   itrs1=2                                       itrs2=4
// RUN:    ompdd0=OMPDistributeDirective             ompdd1=                                       ompdd2=OMPParallelForSimdDirective
// RUN:    ompdp0=distribute                         ompdp1=                                       ompdp2='parallel for simd'
// RUN:    ompdk0=OPRG ompsk0=OSIMP                  ompdk1=OSEQ ompsk1=OSIMP                      ompdk2=OPRG ompsk2=OSEXP
// RUN:    dmp0=DMP0-AIMP,DMP0-AG                    dmp1=DMP1-ASEQ                                dmp2=DMP2-AIMP,DMP2-AW,DMP2-AV
// RUN:    exe=EXE,EXE224)
// RUN:   (accc0=seq                                 accc1='gang worker vector'                    accc2=seq
// RUN:    itrs0=2                                   itrs1=6                                       itrs2=2
// RUN:    ompdd0=                                   ompdd1=OMPDistributeParallelForSimdDirective  ompdd2=
// RUN:    ompdp0=                                   ompdp1='distribute parallel for simd'         ompdp2=
// RUN:    ompdk0=OSEQ ompsk0=OSIMP                  ompdk1=OPRG ompsk1=OSEXP                      ompdk2=OSEQ ompsk2=OSIMP
// RUN:    dmp0=DMP0-ASEQ                            dmp1=DMP1-AIMP,DMP1-AG,DMP1-AW,DMP1-AV        dmp2=DMP2-ASEQ
// RUN:    exe=EXE,EXE262)
// RUN:   (accc0=gang                                accc1=seq                                     accc2=worker
// RUN:    itrs0=2                                   itrs1=2                                       itrs2=2
// RUN:    ompdd0=OMPDistributeDirective             ompdd1=                                       ompdd2=OMPParallelForDirective
// RUN:    ompdp0=distribute                         ompdp1=                                       ompdp2='parallel for'
// RUN:    ompdk0=OPRG ompsk0=OSIMP                  ompdk1=OSEQ ompsk1=OSIMP                      ompdk2=OPRG ompsk2=OSEXP
// RUN:    dmp0=DMP0-AIMP,DMP0-AG                    dmp1=DMP1-ASEQ                                dmp2=DMP2-AIMP,DMP2-AW
// RUN:    exe=EXE,EXE222)
// RUN:   (accc0=seq                                 accc1='gang worker'                           accc2=seq
// RUN:    itrs0=2                                   itrs1=4                                       itrs2=2
// RUN:    ompdd0=                                   ompdd1=OMPDistributeParallelForDirective      ompdd2=
// RUN:    ompdp0=                                   ompdp1='distribute parallel for'              ompdp2=
// RUN:    ompdk0=OSEQ ompsk0=OSIMP                  ompdk1=OPRG ompsk1=OSEXP                      ompdk2=OSEQ ompsk2=OSIMP
// RUN:    dmp0=DMP0-ASEQ                            dmp1=DMP1-AIMP,DMP1-AG,DMP1-AW                dmp2=DMP2-ASEQ
// RUN:    exe=EXE,EXE242)
// RUN:   (accc0=gang                                accc1=seq                                     accc2=vector
// RUN:    itrs0=2                                   itrs1=2                                       itrs2=2
// RUN:    ompdd0=OMPDistributeDirective             ompdd1=                                       ompdd2=OMPSimdDirective
// RUN:    ompdp0=distribute                         ompdp1=                                       ompdp2='simd'
// RUN:    ompdk0=OPRG ompsk0=OSIMP                  ompdk1=OSEQ ompsk1=OSIMP                      ompdk2=OPRG ompsk2=OSIMP
// RUN:    dmp0=DMP0-AIMP,DMP0-AG                    dmp1=DMP1-ASEQ                                dmp2=DMP2-AIMP,DMP2-AV
// RUN:    exe=EXE,EXE222)
// RUN:   (accc0=seq                                 accc1='gang vector'                           accc2=seq
// RUN:    itrs0=2                                   itrs1=4                                       itrs2=2
// RUN:    ompdd0=                                   ompdd1=OMPDistributeSimdDirective             ompdd2=
// RUN:    ompdp0=                                   ompdp1='distribute simd'                      ompdp2=
// RUN:    ompdk0=OSEQ ompsk0=OSIMP                  ompdk1=OPRG ompsk1=OSIMP                      ompdk2=OSEQ ompsk2=OSIMP
// RUN:    dmp0=DMP0-ASEQ                            dmp1=DMP1-AIMP,DMP1-AG,DMP1-AV                dmp2=DMP2-ASEQ
// RUN:    exe=EXE,EXE242)
// RUN:   (accc0=worker                              accc1=seq                                     accc2=vector
// RUN:    itrs0=2                                   itrs1=2                                       itrs2=2
// RUN:    ompdd0=OMPParallelForDirective            ompdd1=                                       ompdd2=OMPSimdDirective
// RUN:    ompdp0='parallel for'                     ompdp1=                                       ompdp2='simd'
// RUN:    ompdk0=OPRG ompsk0=OSEXP                  ompdk1=OSEQ ompsk1=OSIMP                      ompdk2=OPRG ompsk2=OSIMP
// RUN:    dmp0=DMP0-AIMP,DMP0-AW                    dmp1=DMP1-ASEQ                                dmp2=DMP2-AIMP,DMP2-AV
// RUN:    exe=EXE,EXE222,EXEGR222)
// RUN:   (accc0=seq                                 accc1='worker vector'                         accc2=seq
// RUN:    itrs0=2                                   itrs1=4                                       itrs2=2
// RUN:    ompdd0=                                   ompdd1=OMPParallelForSimdDirective            ompdd2=
// RUN:    ompdp0=                                   ompdp1='parallel for simd'                    ompdp2=
// RUN:    ompdk0=OSEQ ompsk0=OSIMP                  ompdk1=OPRG ompsk1=OSEXP                      ompdk2=OSEQ ompsk2=OSIMP
// RUN:    dmp0=DMP0-ASEQ                            dmp1=DMP1-AIMP,DMP1-AW,DMP1-AV                dmp2=DMP2-ASEQ
// RUN:    exe=EXE,EXE242,EXEGR242)
// RUN:   (accc0=seq                                 accc1=gang                                    accc2=seq
// RUN:    itrs0=2                                   itrs1=2                                       itrs2=2
// RUN:    ompdd0=                                   ompdd1=OMPDistributeDirective                 ompdd2=
// RUN:    ompdp0=                                   ompdp1=distribute                             ompdp2=
// RUN:    ompdk0=OSEQ ompsk0=OSIMP                  ompdk1=OPRG ompsk1=OSIMP                      ompdk2=OSEQ ompsk2=OSIMP
// RUN:    dmp0=DMP0-ASEQ                            dmp1=DMP1-AIMP,DMP1-AG                        dmp2=DMP2-ASEQ
// RUN:    exe=EXE,EXE222)
// RUN:   (accc0=seq                                 accc1=worker                                  accc2=seq
// RUN:    itrs0=2                                   itrs1=2                                       itrs2=2
// RUN:    ompdd0=                                   ompdd1=OMPParallelForDirective                ompdd2=
// RUN:    ompdp0=                                   ompdp1='parallel for'                         ompdp2=
// RUN:    ompdk0=OSEQ ompsk0=OSIMP                  ompdk1=OPRG ompsk1=OSEXP                      ompdk2=OSEQ ompsk2=OSIMP
// RUN:    dmp0=DMP0-ASEQ                            dmp1=DMP1-AIMP,DMP1-AW                        dmp2=DMP2-ASEQ
// RUN:    exe=EXE,EXE222,EXEGR222)
// RUN:   (accc0=seq                                 accc1=vector                                  accc2=seq
// RUN:    itrs0=2                                   itrs1=2                                       itrs2=2
// RUN:    ompdd0=                                   ompdd1=OMPParallelForSimdDirective            ompdd2=
// RUN:    ompdp0=                                   ompdp1='parallel for simd num_threads(1)'     ompdp2=
// RUN:    ompdk0=OSEQ ompsk0=OSIMP                  ompdk1=OPRG ompsk1=OSEXP                      ompdk2=OSEQ ompsk2=OSIMP
// RUN:    dmp0=DMP0-ASEQ                            dmp1=DMP1-AIMP,DMP1-AV,DMP1-ONT1              dmp2=DMP2-ASEQ
// RUN:    exe=EXE,EXE222,EXEGR222)
// RUN: }

// Check ASTDumper.
//
// RUN: %for loop-clauses {
// RUN:   %clang -Xclang -verify -Xclang -ast-dump -fsyntax-only -fopenacc %s \
// RUN:          -DACCC0=%'accc0' -DACCC1=%'accc1' -DACCC2=%'accc2' \
// RUN:          -DITRS0=%[itrs0] -DITRS1=%[itrs1] -DITRS2=%[itrs2] \
// RUN:   | FileCheck %s \
// RUN:       -check-prefixes=DMP \
// RUN:       -check-prefixes=DMP0-%[ompdk0],DMP0-%[ompdk0]-%[ompsk0],%[dmp0] \
// RUN:       -check-prefixes=DMP1-%[ompdk1],DMP1-%[ompdk1]-%[ompsk1],%[dmp1] \
// RUN:       -check-prefixes=DMP2-%[ompdk2],DMP2-%[ompdk2]-%[ompsk2],%[dmp2] \
// RUN:       -DOMPDD0=%'ompdd0' -DOMPDD1=%'ompdd1' -DOMPDD2=%'ompdd2'
// RUN: }

// Check -ast-print and -fopenacc-print.
//
// RUN: %clang -Xclang -verify -Xclang -ast-print -fsyntax-only %s \
// RUN:        -DITRS0=2 -DITRS1=2 -DITRS2=2 \
// RUN: | FileCheck -check-prefixes=PRT %s
//
// RUN: %data prints {
// RUN:   (print='-Xclang -ast-print -fsyntax-only -fopenacc' prt=PRT,PRT-A)
// RUN:   (print=-fopenacc-print=acc     prt=PRT,PRT-A)
// RUN:   (print=-fopenacc-print=omp     prt=PRT,PRT-O,PRT-O-%[ompdk0]0,PRT-O-%[ompdk0]0-%[ompsk0]0,PRT-O-%[ompdk1]1,PRT-O-%[ompdk1]1-%[ompsk1]1,PRT-O-%[ompdk2]2,PRT-O-%[ompdk2]2-%[ompsk2]2)
// RUN:   (print=-fopenacc-print=acc-omp prt=PRT,PRT-A,PRT-AOX,PRT-AOX-%[ompdk0]0,PRT-AOX-%[ompdk0]0-%[ompsk0]0,PRT-AOX-%[ompdk1]1,PRT-AOX-%[ompdk1]1-%[ompsk1]1,PRT-AOX-%[ompdk2]2,PRT-AOX-%[ompdk2]2-%[ompsk2]2,PRT-AO,PRT-AO-%[ompdk0]0,PRT-AO-%[ompdk0]0-%[ompsk0]0,PRT-AO-%[ompdk1]1,PRT-AO-%[ompdk1]1-%[ompsk1]1,PRT-AO-%[ompdk2]2,PRT-AO-%[ompdk2]2-%[ompsk2]2)
// RUN:   (print=-fopenacc-print=omp-acc prt=PRT,PRT-O,PRT-O-%[ompdk0]0,PRT-O-%[ompdk0]0-%[ompsk0]0,PRT-O-%[ompdk1]1,PRT-O-%[ompdk1]1-%[ompsk1]1,PRT-O-%[ompdk2]2,PRT-O-%[ompdk2]2-%[ompsk2]2,PRT-AOX,PRT-AOX-%[ompdk0]0,PRT-AOX-%[ompdk0]0-%[ompsk0]0,PRT-AOX-%[ompdk1]1,PRT-AOX-%[ompdk1]1-%[ompsk1]1,PRT-AOX-%[ompdk2]2,PRT-AOX-%[ompdk2]2-%[ompsk2]2,PRT-OA,PRT-OA-%[ompdk0]0,PRT-OA-%[ompdk0]0-%[ompsk0]0,PRT-OA-%[ompdk1]1,PRT-OA-%[ompdk1]1-%[ompsk1]1,PRT-OA-%[ompdk2]2,PRT-OA-%[ompdk2]2-%[ompsk2]2)
// RUN: }
// RUN: %for loop-clauses {
// RUN:   %for prints {
// RUN:     %clang -Xclang -verify %[print] %s \
// RUN:            -DACCC0=%'accc0' -DACCC1=%'accc1' -DACCC2=%'accc2' \
// RUN:            -DITRS0=%'itrs0' -DITRS1=%'itrs1' -DITRS2=%'itrs2' \
// RUN:     | FileCheck -check-prefixes=%[prt] %s \
// RUN:                 -DACCC0=%'accc0'   -DACCC1=%'accc1'   -DACCC2=%'accc2' \
// RUN:                 -DOMPDP0=%'ompdp0' -DOMPDP1=%'ompdp1' -DOMPDP2=%'ompdp2'
// RUN:   }
// RUN: }

// Check ASTWriterStmt, ASTReaderStmt, StmtPrinter, and
// ACCLoopDirective::CreateEmpty (used by ASTReaderStmt).  Some data related to
// printing (where to print comments about discarded directives) is serialized
// and deserialized, so it's worthwhile to try all OpenACC printing modes.
//
// RUN: %for loop-clauses {
// RUN:   %clang -Xclang -verify -fopenacc -emit-ast %s -o %t.ast \
// RUN:          -DACCC0=%'accc0' -DACCC1=%'accc1' -DACCC2=%'accc2' \
// RUN:          -DITRS0=%[itrs0] -DITRS1=%[itrs1] -DITRS2=%[itrs2]
// RUN:   %for prints {
// RUN:     %clang_cc1 -ast-print %t.ast 2>&1 \
// RUN:     | FileCheck -check-prefixes=PRT,PRT-A %s \
// RUN:                 -DACCC0=%'accc0'   -DACCC1=%'accc1'   -DACCC2=%'accc2' \
// RUN:                 -DOMPDP0=%'ompdp0' -DOMPDP1=%'ompdp1' -DOMPDP2=%'ompdp2'
// RUN:   }
// RUN: }

// Can we -ast-print the OpenMP source code, compile, and run it successfully?
//
// RUN: %for loop-clauses {
// RUN:   %clang -Xclang -verify -fopenacc-print=omp %s > %t-omp.c \
// RUN:          -DACCC0=%'accc0' -DACCC1=%'accc1' -DACCC2=%'accc2' \
// RUN:          -DITRS0=%'itrs0' -DITRS1=%'itrs1' -DITRS2=%'itrs2'
// RUN:   echo "// expected""-no-diagnostics" >> %t-omp.c
// RUN:   %clang -Xclang -verify -fopenmp -o %t %t-omp.c
// RUN:   %t | FileCheck -check-prefixes=%'exe' %s \
// RUN:        -DACCC0=%'accc0' -DACCC1=%'accc1' -DACCC2=%'accc2'
// RUN: }

// Check execution with normal compilation.
//
// RUN: %for loop-clauses {
// RUN:   %clang -Xclang -verify -fopenacc %s -o %t \
// RUN:          -DACCC0=%'accc0' -DACCC1=%'accc1' -DACCC2=%'accc2' \
// RUN:          -DITRS0=%'itrs0' -DITRS1=%'itrs1' -DITRS2=%'itrs2'
// RUN:   %t 2 2>&1 \
// RUN:   | FileCheck -check-prefixes=%'exe' %s \
// RUN:               -DACCC0=%'accc0' -DACCC1=%'accc1' -DACCC2=%'accc2'
// RUN: }

// END.

// expected-no-diagnostics

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define XSTR(X) STR(X)
#define STR(X) #X

#ifndef ITRS0
# define ITRS0 0
# error ITRS0 is undefined
#endif
#ifndef ITRS1
# define ITRS1 0
# error ITRS1 is undefined
#endif
#ifndef ITRS2
# define ITRS2 0
# error ITRS2 is undefined
#endif

// PRT: int main() {
int main() {
  // PRT-NEXT: printf
  // EXE: [[ACCC0]] > [[ACCC1]] > [[ACCC2]]
  printf("%s > %s > %s\n", XSTR(ACCC0), XSTR(ACCC1), XSTR(ACCC2));

  // DMP:      ACCParallelDirective
  // DMP-NEXT:   ACCNum_gangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   impl: OMPTargetTeamsDirective
  // DMP-NEXT:     OMPNum_teamsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2){{$}}
  #pragma acc parallel num_gangs(2)
  // PRT-NEXT: {
  {
    // DMP:            ACCLoopDirective
    // DMP0-AG-NEXT:     ACCGangClause
    // DMP0-AW-NEXT:     ACCWorkerClause
    // DMP0-AV-NEXT:     ACCVectorClause
    // DMP0-ASEQ-NEXT:   ACCSeqClause
    // DMP0-AIMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP0-OPRG-NEXT:   impl: [[OMPDD0]]
    // DMP0-ONT1-NEXT:     OMPNum_threadsClause
    // DMP0-ONT1-NEXT:       IntegerLiteral {{.*}} 'int' 1
    // DMP0-OPRG:          ForStmt
    // DMP0-OSEQ-NEXT:   impl: ForStmt
    //
    // Print uncommented directive.
    // PRT-A-NEXT:        {{^ *}}#pragma acc loop [[ACCC0]]
    // PRT-AO-OSEQ0-SAME: {{^}} // discarded in OpenMP translation
    // PRT-A-SAME:        {{^$}}
    // PRT-O-OPRG0-NEXT:  {{^ *}}#pragma omp [[OMPDP0]]{{$}}
    //
    // Print commented directive.
    // PRT-AO-OPRG0-NEXT: {{^ *}}// #pragma omp [[OMPDP0]]{{$}}
    // PRT-OA-NEXT:       {{^ *}}// #pragma acc loop [[ACCC0]]
    // PRT-OA-OSEQ0-SAME: {{^}} // discarded in OpenMP translation
    // PRT-OA-SAME:       {{^$}}
    //
    // Print attached statement.
    // PRT-NEXT: for (int i = 0; i < {{.*}}; ++i) {
    #pragma acc loop ACCC0
    for (int i = 0; i < ITRS0; ++i) {
      // DMP:                  ACCLoopDirective
      // DMP1-AG-NEXT:           ACCGangClause
      // DMP1-AW-NEXT:           ACCWorkerClause
      // DMP1-AV-NEXT:           ACCVectorClause
      // DMP1-ASEQ-NEXT:         ACCSeqClause
      // DMP1-AIMP-NEXT:         ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:               ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:                 DeclRefExpr {{.*}} 'i' 'int'
      // DMP1-OPRG-NEXT:         impl: [[OMPDD1]]
      // DMP1-ONT1-NEXT:           OMPNum_threadsClause
      // DMP1-ONT1-NEXT:             IntegerLiteral {{.*}} 'int' 1
      // DMP1-OPRG-NEXT:           OMPSharedClause
      // DMP1-OPRG-OSIMP-SAME:       <implicit>
      // DMP1-OPRG-OSEXP-NOT:        <implicit>
      // DMP1-OPRG-NEXT:             DeclRefExpr {{.*}} 'i' 'int'
      // DMP1-OPRG:                ForStmt
      // DMP1-OSEQ-NEXT:         impl: ForStmt
      //
      // Print uncommented directive.
      // PRT-A-NEXT:              {{^ *}}#pragma acc loop [[ACCC1]]
      // PRT-AO-OSEQ1-SAME:       {{^}} // discarded in OpenMP translation
      // PRT-A-SAME:              {{^$}}
      // PRT-O-OPRG1-OSIMP1-NEXT: {{^ *}}#pragma omp [[OMPDP1]]{{$}}
      // PRT-O-OPRG1-OSEXP1-NEXT: {{^ *}}#pragma omp [[OMPDP1]] shared(i){{$}}
      //
      // Print commented directive.
      // PRT-AO-OPRG1-OSIMP1-NEXT: {{^ *}}// #pragma omp [[OMPDP1]]{{$}}
      // PRT-AO-OPRG1-OSEXP1-NEXT: {{^ *}}// #pragma omp [[OMPDP1]] shared(i){{$}}
      // PRT-OA-NEXT:              {{^ *}}// #pragma acc loop [[ACCC1]]
      // PRT-OA-OSEQ1-SAME:        {{^}} // discarded in OpenMP translation
      // PRT-OA-SAME:              {{^$}}
      //
      // Print attached statement.
      // PRT-NEXT: for (int j = 0; j < {{.*}}; ++j) {
      #pragma acc loop ACCC1
      for (int j = 0; j < ITRS1; ++j) {
        // DMP:                  ACCLoopDirective
        // DMP2-AG-NEXT:           ACCGangClause
        // DMP2-AW-NEXT:           ACCWorkerClause
        // DMP2-AV-NEXT:           ACCVectorClause
        // DMP2-ASEQ-NEXT:         ACCSeqClause
        // DMP2-AIMP-NEXT:         ACCIndependentClause {{.*}} <implicit>
        // DMP-NEXT:               ACCSharedClause {{.*}} <implicit>
        // DMP-NEXT:                 DeclRefExpr {{.*}} 'i' 'int'
        // DMP-NEXT:                 DeclRefExpr {{.*}} 'j' 'int'
        // DMP2-OPRG-NEXT:         impl: [[OMPDD2]]
        // DMP2-ONT1-NEXT:           OMPNum_threadsClause
        // DMP2-ONT1-NEXT:             IntegerLiteral {{.*}} 'int' 1
        // DMP2-OPRG-NEXT:           OMPSharedClause
        // DMP2-OPRG-OSIMP-SAME:       <implicit>
        // DMP2-OPRG-OSEXP-NOT:        <implicit>
        // DMP2-OPRG-NEXT:             DeclRefExpr {{.*}} 'i' 'int'
        // DMP2-OPRG-NEXT:             DeclRefExpr {{.*}} 'j' 'int'
        // DMP2-OPRG:                ForStmt
        // DMP2-OSEQ-NEXT:         impl: ForStmt
        //
        // Print uncommented directive.
        // PRT-A-NEXT:              {{^ *}}#pragma acc loop [[ACCC2]]
        // PRT-AO-OSEQ2-SAME:       {{^}} // discarded in OpenMP translation
        // PRT-A-SAME:              {{^$}}
        // PRT-O-OPRG2-OSIMP2-NEXT: {{^ *}}#pragma omp [[OMPDP2]]{{$}}
        // PRT-O-OPRG2-OSEXP2-NEXT: {{^ *}}#pragma omp [[OMPDP2]] shared(i,j){{$}}
        //
        // Print commented directive.
        // PRT-AO-OPRG2-OSIMP2-NEXT: {{^ *}}// #pragma omp [[OMPDP2]]{{$}}
        // PRT-AO-OPRG2-OSEXP2-NEXT: {{^ *}}// #pragma omp [[OMPDP2]] shared(i,j){{$}}
        // PRT-OA-NEXT:              {{^ *}}// #pragma acc loop [[ACCC2]]
        // PRT-OA-OSEQ2-SAME:        {{^}} // discarded in OpenMP translation
        // PRT-OA-SAME:              {{^$}}
        //
        // Print attached statement.
        // PRT-NEXT: for (int k = 0; k < {{.*}}; ++k) {
        #pragma acc loop ACCC2
        for (int k = 0; k < ITRS2; ++k) {
          // PRT-NEXT: printf

          // EXE222-DAG: 0, 0, 0
          // EXE222-DAG: 0, 0, 1
          // EXE222-DAG: 0, 1, 0
          // EXE222-DAG: 0, 1, 1
          // EXE222-DAG: 1, 0, 0
          // EXE222-DAG: 1, 0, 1
          // EXE222-DAG: 1, 1, 0
          // EXE222-DAG: 1, 1, 1

          // EXEGR222-DAG: 0, 0, 0
          // EXEGR222-DAG: 0, 0, 1
          // EXEGR222-DAG: 0, 1, 0
          // EXEGR222-DAG: 0, 1, 1
          // EXEGR222-DAG: 1, 0, 0
          // EXEGR222-DAG: 1, 0, 1
          // EXEGR222-DAG: 1, 1, 0
          // EXEGR222-DAG: 1, 1, 1

          // EXE224-DAG: 0, 0, 0
          // EXE224-DAG: 0, 0, 1
          // EXE224-DAG: 0, 0, 2
          // EXE224-DAG: 0, 0, 3
          // EXE224-DAG: 0, 1, 0
          // EXE224-DAG: 0, 1, 1
          // EXE224-DAG: 0, 1, 2
          // EXE224-DAG: 0, 1, 3
          // EXE224-DAG: 1, 0, 0
          // EXE224-DAG: 1, 0, 1
          // EXE224-DAG: 1, 0, 2
          // EXE224-DAG: 1, 0, 3
          // EXE224-DAG: 1, 1, 0
          // EXE224-DAG: 1, 1, 1
          // EXE224-DAG: 1, 1, 2
          // EXE224-DAG: 1, 1, 3

          // EXE226-DAG: 0, 0, 0
          // EXE226-DAG: 0, 0, 1
          // EXE226-DAG: 0, 0, 2
          // EXE226-DAG: 0, 0, 3
          // EXE226-DAG: 0, 0, 4
          // EXE226-DAG: 0, 0, 5
          // EXE226-DAG: 0, 1, 0
          // EXE226-DAG: 0, 1, 1
          // EXE226-DAG: 0, 1, 2
          // EXE226-DAG: 0, 1, 3
          // EXE226-DAG: 0, 1, 4
          // EXE226-DAG: 0, 1, 5
          // EXE226-DAG: 1, 0, 0
          // EXE226-DAG: 1, 0, 1
          // EXE226-DAG: 1, 0, 2
          // EXE226-DAG: 1, 0, 3
          // EXE226-DAG: 1, 0, 4
          // EXE226-DAG: 1, 0, 5
          // EXE226-DAG: 1, 1, 0
          // EXE226-DAG: 1, 1, 1
          // EXE226-DAG: 1, 1, 2
          // EXE226-DAG: 1, 1, 3
          // EXE226-DAG: 1, 1, 4
          // EXE226-DAG: 1, 1, 5

          // EXE242-DAG: 0, 0, 0
          // EXE242-DAG: 0, 0, 1
          // EXE242-DAG: 0, 1, 0
          // EXE242-DAG: 0, 1, 1
          // EXE242-DAG: 0, 2, 0
          // EXE242-DAG: 0, 2, 1
          // EXE242-DAG: 0, 3, 0
          // EXE242-DAG: 0, 3, 1
          // EXE242-DAG: 1, 0, 0
          // EXE242-DAG: 1, 0, 1
          // EXE242-DAG: 1, 1, 0
          // EXE242-DAG: 1, 1, 1
          // EXE242-DAG: 1, 2, 0
          // EXE242-DAG: 1, 2, 1
          // EXE242-DAG: 1, 3, 0
          // EXE242-DAG: 1, 3, 1

          // EXEGR242-DAG: 0, 0, 0
          // EXEGR242-DAG: 0, 0, 1
          // EXEGR242-DAG: 0, 1, 0
          // EXEGR242-DAG: 0, 1, 1
          // EXEGR242-DAG: 0, 2, 0
          // EXEGR242-DAG: 0, 2, 1
          // EXEGR242-DAG: 0, 3, 0
          // EXEGR242-DAG: 0, 3, 1
          // EXEGR242-DAG: 1, 0, 0
          // EXEGR242-DAG: 1, 0, 1
          // EXEGR242-DAG: 1, 1, 0
          // EXEGR242-DAG: 1, 1, 1
          // EXEGR242-DAG: 1, 2, 0
          // EXEGR242-DAG: 1, 2, 1
          // EXEGR242-DAG: 1, 3, 0
          // EXEGR242-DAG: 1, 3, 1

          // EXE262-DAG: 0, 0, 0
          // EXE262-DAG: 0, 0, 1
          // EXE262-DAG: 0, 1, 0
          // EXE262-DAG: 0, 1, 1
          // EXE262-DAG: 0, 2, 0
          // EXE262-DAG: 0, 2, 1
          // EXE262-DAG: 0, 3, 0
          // EXE262-DAG: 0, 3, 1
          // EXE262-DAG: 0, 4, 0
          // EXE262-DAG: 0, 4, 1
          // EXE262-DAG: 0, 5, 0
          // EXE262-DAG: 0, 5, 1
          // EXE262-DAG: 1, 0, 0
          // EXE262-DAG: 1, 0, 1
          // EXE262-DAG: 1, 1, 0
          // EXE262-DAG: 1, 1, 1
          // EXE262-DAG: 1, 2, 0
          // EXE262-DAG: 1, 2, 1
          // EXE262-DAG: 1, 3, 0
          // EXE262-DAG: 1, 3, 1
          // EXE262-DAG: 1, 4, 0
          // EXE262-DAG: 1, 4, 1
          // EXE262-DAG: 1, 5, 0
          // EXE262-DAG: 1, 5, 1

          // EXE422-DAG: 0, 0, 0
          // EXE422-DAG: 0, 0, 1
          // EXE422-DAG: 0, 1, 0
          // EXE422-DAG: 0, 1, 1
          // EXE422-DAG: 1, 0, 0
          // EXE422-DAG: 1, 0, 1
          // EXE422-DAG: 1, 1, 0
          // EXE422-DAG: 1, 1, 1
          // EXE422-DAG: 2, 0, 0
          // EXE422-DAG: 2, 0, 1
          // EXE422-DAG: 2, 1, 0
          // EXE422-DAG: 2, 1, 1
          // EXE422-DAG: 3, 0, 0
          // EXE422-DAG: 3, 0, 1
          // EXE422-DAG: 3, 1, 0
          // EXE422-DAG: 3, 1, 1
          printf("%d, %d, %d\n", i, j, k);
        // PRT-NEXT: }
        }
      // PRT-NEXT: }
      }
    // PRT-NEXT: }
    }
  // PRT-NEXT: }
  }

  // Close off EXE*-DAGs.
  // PRT-NEXT: printf
  // EXE-NOT: {{.}}
  // EXE-NEXT: END
  printf("END\n");

  // PRT-NEXT: return 0;
  return 0;

// EXE-NOT: {{.}}
// PRT-NEXT: }
}
