// Check nested acc loops.
//
// When CMB is not set, this file checks cases when the outermost "acc loop"
// is separate from the enclosing "acc parallel".
//
// When CMB is set, it combines each "acc parallel" with its outermost "acc
// loop" directive in order to check the same cases but for combined "acc
// parallel loop" directives.
//
// Abbreviations:
//   A     = OpenACC
//   AO    = commented OpenMP is printed after OpenACC
//   O     = OpenMP
//   OA    = commented OpenACC is printed after OpenMP
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
// RUN: %data cmbs {
// RUN:   (cmb-cflags=      cmb=SEP)
// RUN:   (cmb-cflags=-DCMB cmb=CMB)
// RUN: }
// RUN: %data loop-clauses {
// RUN:   (accc0=gang                                accc1=worker                                  accc2=vector
// RUN:    itrs0=2                                   itrs1=2                                       itrs2=2
// RUN:    ompdd0=OMPDistributeDirective             ompdd1=OMPParallelForDirective                ompdd2=OMPSimdDirective
// RUN:    ompdp0=distribute                         ompdp1='parallel for'                         ompdp2='simd'
// RUN:    ompdk0=OPRG ompsk0=OSIMP                  ompdk1=OPRG ompsk1=OSEXP                      ompdk2=OPRG ompsk2=OSIMP
// RUN:    dmp0=DMP0-AIMP,DMP0-AG,DMP0-%[cmb]-AIMP,DMP0-%[cmb]-AG
// RUN:                                              dmp1=DMP1-AIMP,DMP1-AW                        dmp2=DMP2-AIMP,DMP2-AV
// RUN:    exe=EXE222)
// RUN:   (accc0='gang worker'                       accc1=seq                                     accc2=vector
// RUN:    itrs0=4                                   itrs1=2                                       itrs2=2
// RUN:    ompdd0=OMPDistributeParallelForDirective  ompdd1=                                       ompdd2=OMPSimdDirective
// RUN:    ompdp0='distribute parallel for'          ompdp1=                                       ompdp2='simd'
// RUN:    ompdk0=OPRG ompsk0=OSEXP                  ompdk1=OSEQ ompsk1=OSIMP                      ompdk2=OPRG ompsk2=OSIMP
// RUN:    dmp0=DMP0-AIMP,DMP0-AG,DMP0-AW,DMP0-%[cmb]-AIMP,DMP0-%[cmb]-AG,DMP0-%[cmb]-AW
// RUN:                                              dmp1=DMP1-ASEQ                                dmp2=DMP2-AIMP,DMP2-AV
// RUN:    exe=EXE,EXE422)
// RUN:   (accc0=gang                                accc1=seq                                     accc2='worker vector'
// RUN:    itrs0=2                                   itrs1=2                                       itrs2=4
// RUN:    ompdd0=OMPDistributeDirective             ompdd1=                                       ompdd2=OMPParallelForSimdDirective
// RUN:    ompdp0=distribute                         ompdp1=                                       ompdp2='parallel for simd'
// RUN:    ompdk0=OPRG ompsk0=OSIMP                  ompdk1=OSEQ ompsk1=OSIMP                      ompdk2=OPRG ompsk2=OSEXP
// RUN:    dmp0=DMP0-AIMP,DMP0-AG,DMP0-%[cmb]-AIMP,DMP0-%[cmb]-AG
// RUN:                                              dmp1=DMP1-ASEQ                                dmp2=DMP2-AIMP,DMP2-AW,DMP2-AV
// RUN:    exe=EXE,EXE224)
// RUN:   (accc0=seq                                 accc1='gang worker vector'                    accc2=seq
// RUN:    itrs0=2                                   itrs1=6                                       itrs2=2
// RUN:    ompdd0=                                   ompdd1=OMPDistributeParallelForSimdDirective  ompdd2=
// RUN:    ompdp0=                                   ompdp1='distribute parallel for simd'         ompdp2=
// RUN:    ompdk0=OSEQ ompsk0=OSIMP                  ompdk1=OPRG ompsk1=OSEXP                      ompdk2=OSEQ ompsk2=OSIMP
// RUN:    dmp0=DMP0-ASEQ,DMP0-%[cmb]-ASEQ           dmp1=DMP1-AIMP,DMP1-AG,DMP1-AW,DMP1-AV        dmp2=DMP2-ASEQ
// RUN:    exe=EXE,EXE262)
// RUN:   (accc0=gang                                accc1=seq                                     accc2=worker
// RUN:    itrs0=2                                   itrs1=2                                       itrs2=2
// RUN:    ompdd0=OMPDistributeDirective             ompdd1=                                       ompdd2=OMPParallelForDirective
// RUN:    ompdp0=distribute                         ompdp1=                                       ompdp2='parallel for'
// RUN:    ompdk0=OPRG ompsk0=OSIMP                  ompdk1=OSEQ ompsk1=OSIMP                      ompdk2=OPRG ompsk2=OSEXP
// RUN:    dmp0=DMP0-AIMP,DMP0-AG,DMP0-%[cmb]-AIMP,DMP0-%[cmb]-AG
// RUN:                                              dmp1=DMP1-ASEQ                                dmp2=DMP2-AIMP,DMP2-AW
// RUN:    exe=EXE,EXE222)
// RUN:   (accc0=seq                                 accc1='gang worker'                           accc2=seq
// RUN:    itrs0=2                                   itrs1=4                                       itrs2=2
// RUN:    ompdd0=                                   ompdd1=OMPDistributeParallelForDirective      ompdd2=
// RUN:    ompdp0=                                   ompdp1='distribute parallel for'              ompdp2=
// RUN:    ompdk0=OSEQ ompsk0=OSIMP                  ompdk1=OPRG ompsk1=OSEXP                      ompdk2=OSEQ ompsk2=OSIMP
// RUN:    dmp0=DMP0-ASEQ,DMP0-%[cmb]-ASEQ           dmp1=DMP1-AIMP,DMP1-AG,DMP1-AW                dmp2=DMP2-ASEQ
// RUN:    exe=EXE,EXE242)
// RUN:   (accc0=gang                                accc1=seq                                     accc2=vector
// RUN:    itrs0=2                                   itrs1=2                                       itrs2=2
// RUN:    ompdd0=OMPDistributeDirective             ompdd1=                                       ompdd2=OMPSimdDirective
// RUN:    ompdp0=distribute                         ompdp1=                                       ompdp2='simd'
// RUN:    ompdk0=OPRG ompsk0=OSIMP                  ompdk1=OSEQ ompsk1=OSIMP                      ompdk2=OPRG ompsk2=OSIMP
// RUN:    dmp0=DMP0-AIMP,DMP0-AG,DMP0-%[cmb]-AIMP,DMP0-%[cmb]-AG
// RUN:                                              dmp1=DMP1-ASEQ                                dmp2=DMP2-AIMP,DMP2-AV
// RUN:    exe=EXE,EXE222)
// RUN:   (accc0=seq                                 accc1='gang vector'                           accc2=seq
// RUN:    itrs0=2                                   itrs1=4                                       itrs2=2
// RUN:    ompdd0=                                   ompdd1=OMPDistributeSimdDirective             ompdd2=
// RUN:    ompdp0=                                   ompdp1='distribute simd'                      ompdp2=
// RUN:    ompdk0=OSEQ ompsk0=OSIMP                  ompdk1=OPRG ompsk1=OSIMP                      ompdk2=OSEQ ompsk2=OSIMP
// RUN:    dmp0=DMP0-ASEQ,DMP0-%[cmb]-ASEQ           dmp1=DMP1-AIMP,DMP1-AG,DMP1-AV                dmp2=DMP2-ASEQ
// RUN:    exe=EXE,EXE242)
// RUN:   (accc0=worker                              accc1=seq                                     accc2=vector
// RUN:    itrs0=2                                   itrs1=2                                       itrs2=2
// RUN:    ompdd0=OMPParallelForDirective            ompdd1=                                       ompdd2=OMPSimdDirective
// RUN:    ompdp0='parallel for'                     ompdp1=                                       ompdp2='simd'
// RUN:    ompdk0=OPRG ompsk0=OSEXP                  ompdk1=OSEQ ompsk1=OSIMP                      ompdk2=OPRG ompsk2=OSIMP
// RUN:    dmp0=DMP0-AIMP,DMP0-AW,DMP0-%[cmb]-AIMP,DMP0-%[cmb]-AW
// RUN:                                              dmp1=DMP1-ASEQ                                dmp2=DMP2-AIMP,DMP2-AV
// RUN:    exe=EXE,EXE222,EXEGR222)
// RUN:   (accc0=seq                                 accc1='worker vector'                         accc2=seq
// RUN:    itrs0=2                                   itrs1=4                                       itrs2=2
// RUN:    ompdd0=                                   ompdd1=OMPParallelForSimdDirective            ompdd2=
// RUN:    ompdp0=                                   ompdp1='parallel for simd'                    ompdp2=
// RUN:    ompdk0=OSEQ ompsk0=OSIMP                  ompdk1=OPRG ompsk1=OSEXP                      ompdk2=OSEQ ompsk2=OSIMP
// RUN:    dmp0=DMP0-ASEQ,DMP0-%[cmb]-ASEQ           dmp1=DMP1-AIMP,DMP1-AW,DMP1-AV                dmp2=DMP2-ASEQ
// RUN:    exe=EXE,EXE242,EXEGR242)
// RUN:   (accc0=seq                                 accc1=gang                                    accc2=seq
// RUN:    itrs0=2                                   itrs1=2                                       itrs2=2
// RUN:    ompdd0=                                   ompdd1=OMPDistributeDirective                 ompdd2=
// RUN:    ompdp0=                                   ompdp1=distribute                             ompdp2=
// RUN:    ompdk0=OSEQ ompsk0=OSIMP                  ompdk1=OPRG ompsk1=OSIMP                      ompdk2=OSEQ ompsk2=OSIMP
// RUN:    dmp0=DMP0-ASEQ,DMP0-%[cmb]-ASEQ           dmp1=DMP1-AIMP,DMP1-AG                        dmp2=DMP2-ASEQ
// RUN:    exe=EXE,EXE222)
// RUN:   (accc0=seq                                 accc1=worker                                  accc2=seq
// RUN:    itrs0=2                                   itrs1=2                                       itrs2=2
// RUN:    ompdd0=                                   ompdd1=OMPParallelForDirective                ompdd2=
// RUN:    ompdp0=                                   ompdp1='parallel for'                         ompdp2=
// RUN:    ompdk0=OSEQ ompsk0=OSIMP                  ompdk1=OPRG ompsk1=OSEXP                      ompdk2=OSEQ ompsk2=OSIMP
// RUN:    dmp0=DMP0-ASEQ,DMP0-%[cmb]-ASEQ           dmp1=DMP1-AIMP,DMP1-AW                        dmp2=DMP2-ASEQ
// RUN:    exe=EXE,EXE222,EXEGR222)
// RUN:   (accc0=seq                                 accc1=vector                                  accc2=seq
// RUN:    itrs0=2                                   itrs1=2                                       itrs2=2
// RUN:    ompdd0=                                   ompdd1=OMPParallelForSimdDirective            ompdd2=
// RUN:    ompdp0=                                   ompdp1='parallel for simd num_threads(1)'     ompdp2=
// RUN:    ompdk0=OSEQ ompsk0=OSIMP                  ompdk1=OPRG ompsk1=OSEXP                      ompdk2=OSEQ ompsk2=OSIMP
// RUN:    dmp0=DMP0-ASEQ,DMP0-%[cmb]-ASEQ           dmp1=DMP1-AIMP,DMP1-AV,DMP1-ONT1              dmp2=DMP2-ASEQ
// RUN:    exe=EXE,EXE222,EXEGR222)
// RUN: }

// Check ASTDumper.
//
// RUN: %for cmbs {
// RUN:   %for loop-clauses {
// RUN:     %clang -Xclang -verify -Xclang -ast-dump -fsyntax-only -fopenacc %s \
// RUN:            -DACCC0=%'accc0' -DACCC1=%'accc1' -DACCC2=%'accc2' \
// RUN:            -DITRS0=%[itrs0] -DITRS1=%[itrs1] -DITRS2=%[itrs2] \
// RUN:            %[cmb-cflags] \
// RUN:     | FileCheck %s \
// RUN:         -check-prefixes=DMP,DMP-%[cmb] \
// RUN:         -check-prefixes=%[dmp0],DMP0-%[ompdk0],DMP0-%[ompdk0]-%[ompsk0] \
// RUN:         -check-prefixes=DMP0-%[cmb]-%[ompdk0],DMP0-%[cmb]-%[ompdk0]-%[cmb]-%[ompsk0] \
// RUN:         -check-prefixes=%[dmp1],DMP1-%[ompdk1],DMP1-%[ompdk1]-%[ompsk1] \
// RUN:         -check-prefixes=%[dmp2],DMP2-%[ompdk2],DMP2-%[ompdk2]-%[ompsk2] \
// RUN:         -DOMPDD0=%'ompdd0' -DOMPDD1=%'ompdd1' -DOMPDD2=%'ompdd2'
// RUN:   }
// RUN: }

// Check -ast-print and -fopenacc[-ast]-print.
//
// RUN: %clang -Xclang -verify -Xclang -ast-print -fsyntax-only %s \
// RUN:        -DITRS0=2 -DITRS1=2 -DITRS2=2 \
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
// RUN:   (prt='-Xclang -ast-print -fsyntax-only -fopenacc' ACCC0="%'accc0'" ACCC1="%'accc1'" ACCC2="%'accc2'" prt-chk=PRT,PRT-%[cmb],PRT-A,PRT-A-%[cmb])
// RUN:   (prt=-fopenacc-ast-print=acc                      ACCC0="%'accc0'" ACCC1="%'accc1'" ACCC2="%'accc2'" prt-chk=PRT,PRT-%[cmb],PRT-A,PRT-A-%[cmb])
// RUN:   (prt=-fopenacc-ast-print=omp                      ACCC0="%'accc0'" ACCC1="%'accc1'" ACCC2="%'accc2'" prt-chk=PRT,PRT-%[cmb],PRT-O,PRT-O-%[cmb],PRT-O-%[ompdk0]0,PRT-O-%[ompdk0]0-%[ompsk0]0,PRT-O-%[cmb]-%[ompdk0]0,PRT-O-%[cmb]-%[ompdk0]0-%[ompsk0]0,PRT-O-%[ompdk1]1,PRT-O-%[ompdk1]1-%[ompsk1]1,PRT-O-%[ompdk2]2,PRT-O-%[ompdk2]2-%[ompsk2]2)
// RUN:   (prt=-fopenacc-ast-print=acc-omp                  ACCC0="%'accc0'" ACCC1="%'accc1'" ACCC2="%'accc2'" prt-chk=PRT,PRT-%[cmb],PRT-A,PRT-A-%[cmb],PRT-AO,PRT-AO-%[cmb],PRT-AO-%[ompdk0]0,PRT-AO-%[ompdk0]0-%[ompsk0]0,PRT-AO-%[cmb]-%[ompdk0]0,PRT-AO-%[cmb]-%[ompdk0]0-%[ompsk0]0,PRT-AO-%[ompdk1]1,PRT-AO-%[ompdk1]1-%[ompsk1]1,PRT-AO-%[ompdk2]2,PRT-AO-%[ompdk2]2-%[ompsk2]2)
// RUN:   (prt=-fopenacc-ast-print=omp-acc                  ACCC0="%'accc0'" ACCC1="%'accc1'" ACCC2="%'accc2'" prt-chk=PRT,PRT-%[cmb],PRT-O,PRT-O-%[cmb],PRT-O-%[ompdk0]0,PRT-O-%[ompdk0]0-%[ompsk0]0,PRT-O-%[cmb]-%[ompdk0]0,PRT-O-%[cmb]-%[ompdk0]0-%[ompsk0]0,PRT-O-%[ompdk1]1,PRT-O-%[ompdk1]1-%[ompsk1]1,PRT-O-%[ompdk2]2,PRT-O-%[ompdk2]2-%[ompsk2]2,PRT-OA,PRT-OA-%[cmb],PRT-OA-%[ompdk0]0,PRT-OA-%[ompdk0]0-%[ompsk0]0,PRT-OA-%[cmb]-%[ompdk0]0,PRT-OA-%[cmb]-%[ompdk0]0-%[ompsk0]0,PRT-OA-%[ompdk1]1,PRT-OA-%[ompdk1]1-%[ompsk1]1,PRT-OA-%[ompdk2]2,PRT-OA-%[ompdk2]2-%[ompsk2]2)
// RUN:   (prt=-fopenacc-print=acc                          ACCC0=ACCC0      ACCC1=ACCC1      ACCC2=ACCC2      prt-chk=PRT-PRE,PRT-PRE-%[cmb],PRT,PRT-%[cmb],PRT-A,PRT-A-%[cmb])
// RUN:   (prt=-fopenacc-print=omp                          ACCC0=ACCC0      ACCC1=ACCC1      ACCC2=ACCC2      prt-chk=PRT-PRE,PRT-PRE-%[cmb],PRT,PRT-%[cmb],PRT-O,PRT-O-%[cmb],PRT-O-%[ompdk0]0,PRT-O-%[ompdk0]0-%[ompsk0]0,PRT-O-%[cmb]-%[ompdk0]0,PRT-O-%[cmb]-%[ompdk0]0-%[ompsk0]0,PRT-O-%[ompdk1]1,PRT-O-%[ompdk1]1-%[ompsk1]1,PRT-O-%[ompdk2]2,PRT-O-%[ompdk2]2-%[ompsk2]2)
// RUN:   (prt=-fopenacc-print=acc-omp                      ACCC0=ACCC0      ACCC1=ACCC1      ACCC2=ACCC2      prt-chk=PRT-PRE,PRT-PRE-%[cmb],PRT,PRT-%[cmb],PRT-A,PRT-A-%[cmb],PRT-AO,PRT-AO-%[cmb],PRT-AO-%[ompdk0]0,PRT-AO-%[ompdk0]0-%[ompsk0]0,PRT-AO-%[cmb]-%[ompdk0]0,PRT-AO-%[cmb]-%[ompdk0]0-%[ompsk0]0,PRT-AO-%[ompdk1]1,PRT-AO-%[ompdk1]1-%[ompsk1]1,PRT-AO-%[ompdk2]2,PRT-AO-%[ompdk2]2-%[ompsk2]2)
// RUN:   (prt=-fopenacc-print=omp-acc                      ACCC0=ACCC0      ACCC1=ACCC1      ACCC2=ACCC2      prt-chk=PRT-PRE,PRT-PRE-%[cmb],PRT,PRT-%[cmb],PRT-O,PRT-O-%[cmb],PRT-O-%[ompdk0]0,PRT-O-%[ompdk0]0-%[ompsk0]0,PRT-O-%[cmb]-%[ompdk0]0,PRT-O-%[cmb]-%[ompdk0]0-%[ompsk0]0,PRT-O-%[ompdk1]1,PRT-O-%[ompdk1]1-%[ompsk1]1,PRT-O-%[ompdk2]2,PRT-O-%[ompdk2]2-%[ompsk2]2,PRT-OA,PRT-OA-%[cmb],PRT-OA-%[ompdk0]0,PRT-OA-%[ompdk0]0-%[ompsk0]0,PRT-OA-%[cmb]-%[ompdk0]0,PRT-OA-%[cmb]-%[ompdk0]0-%[ompsk0]0,PRT-OA-%[ompdk1]1,PRT-OA-%[ompdk1]1-%[ompsk1]1,PRT-OA-%[ompdk2]2,PRT-OA-%[ompdk2]2-%[ompsk2]2)
// RUN: }
// RUN: %for cmbs {
// RUN:   %for loop-clauses {
// RUN:     %for prt-args {
// RUN:       %clang -Xclang -verify %[prt] %t-acc.c \
// RUN:              -DACCC0=%'accc0' -DACCC1=%'accc1' -DACCC2=%'accc2' \
// RUN:              -DITRS0=%'itrs0' -DITRS1=%'itrs1' -DITRS2=%'itrs2' \
// RUN:              %[cmb-cflags] \
// RUN:       | FileCheck -check-prefixes=%[prt-chk] %s \
// RUN:                   -DACCC0=%[ACCC0]   -DACCC1=%[ACCC1]   -DACCC2=%[ACCC2] \
// RUN:                   -DOMPDP0=%'ompdp0' -DOMPDP1=%'ompdp1' -DOMPDP2=%'ompdp2'
// RUN:     }
// RUN:   }
// RUN: }

// Check ASTWriterStmt, ASTReaderStmt, StmtPrinter, and
// ACCLoopDirective::CreateEmpty (used by ASTReaderStmt).  Some data related to
// printing (where to print comments about discarded directives) is serialized
// and deserialized, so it's worthwhile to try all OpenACC printing modes.
//
// RUN: %for cmbs {
// RUN:   %for loop-clauses {
// RUN:     %clang -Xclang -verify -fopenacc -emit-ast %t-acc.c -o %t.ast \
// RUN:            -DACCC0=%'accc0' -DACCC1=%'accc1' -DACCC2=%'accc2' \
// RUN:            -DITRS0=%'itrs0' -DITRS1=%'itrs1' -DITRS2=%'itrs2' \
// RUN:            %[cmb-cflags]
// RUN:     %for prt-args {
// RUN:       %clang %[prt] %t.ast 2>&1 \
// RUN:       | FileCheck -check-prefixes=%[prt-chk] %s \
// RUN:                   -DACCC0=%[ACCC0]   -DACCC1=%[ACCC1]   -DACCC2=%[ACCC2] \
// RUN:                   -DOMPDP0=%'ompdp0' -DOMPDP1=%'ompdp1' -DOMPDP2=%'ompdp2'
// RUN:     }
// RUN:   }
// RUN: }

// Can we print the OpenMP source code, compile, and run it successfully?
//
// RUN: %for cmbs {
// RUN:   %for loop-clauses {
// RUN:     %for prt-opts {
// RUN:       %clang -Xclang -verify %[prt-opt]=omp %s > %t-omp.c \
// RUN:              -DACCC0=%'accc0' -DACCC1=%'accc1' -DACCC2=%'accc2' \
// RUN:              -DITRS0=%'itrs0' -DITRS1=%'itrs1' -DITRS2=%'itrs2' \
// RUN:               %[cmb-cflags]
// RUN:       echo "// expected""-no-diagnostics" >> %t-omp.c
// RUN:       %clang -Xclang -verify -fopenmp -o %t %t-omp.c \
// RUN:              -DACCC0=%'accc0' -DACCC1=%'accc1' -DACCC2=%'accc2' \
// RUN:              -DITRS0=%'itrs0' -DITRS1=%'itrs1' -DITRS2=%'itrs2' \
// RUN:               %[cmb-cflags]
// RUN:       %t 2>&1 \
// RUN:       | FileCheck -check-prefixes=%'exe' %s \
// RUN:                   -DACCC0=%'accc0' -DACCC1=%'accc1' -DACCC2=%'accc2'
// RUN:     }
// RUN:   }
// RUN: }

// Check execution with normal compilation.
//
// RUN: %for cmbs {
// RUN:   %for loop-clauses {
// RUN:     %clang -Xclang -verify -fopenacc %s -o %t \
// RUN:            -DACCC0=%'accc0' -DACCC1=%'accc1' -DACCC2=%'accc2' \
// RUN:            -DITRS0=%'itrs0' -DITRS1=%'itrs1' -DITRS2=%'itrs2' \
// RUN:            %[cmb-cflags]
// RUN:     %t 2>&1 \
// RUN:     | FileCheck -check-prefixes=%'exe' %s \
// RUN:                 -DACCC0=%'accc0' -DACCC1=%'accc1' -DACCC2=%'accc2'
// RUN:   }
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

  // DMP-CMB:            ACCParallelLoopDirective
  // DMP-CMB-NEXT:         ACCNum_gangsClause
  // DMP-CMB-NEXT:           IntegerLiteral {{.*}} 'int' 2
  // DMP0-CMB-AG-NEXT:     ACCGangClause
  // DMP0-CMB-AW-NEXT:     ACCWorkerClause
  // DMP0-CMB-AV-NEXT:     ACCVectorClause
  // DMP0-CMB-ASEQ-NEXT:   ACCSeqClause
  // DMP-SEP:              {{^[^a-z]*}}ACCParallelDirective
  // DMP-CMB-NEXT:         effect: ACCParallelDirective
  // DMP-NEXT:               ACCNum_gangsClause
  // DMP-NEXT:                 IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:               impl: OMPTargetTeamsDirective
  // DMP-NEXT:                 OMPNum_teamsClause
  // DMP-NEXT:                   IntegerLiteral {{.*}} 'int' 2
  // DMP:                    ACCLoopDirective
  // DMP0-AG-NEXT:             ACCGangClause
  // DMP0-AW-NEXT:             ACCWorkerClause
  // DMP0-AV-NEXT:             ACCVectorClause
  // DMP0-ASEQ-NEXT:           ACCSeqClause
  // DMP0-AIMP-NEXT:           ACCIndependentClause {{.*}} <implicit>
  // DMP0-OPRG-NEXT:           impl: [[OMPDD0]]
  // DMP0-ONT1-NEXT:             OMPNum_threadsClause
  // DMP0-ONT1-NEXT:               IntegerLiteral {{.*}} 'int' 1
  // DMP0-OPRG:                  ForStmt
  // DMP0-OSEQ-NEXT:           impl: ForStmt
  //
  //
  // PRT-PRE-SEP-NEXT: #if !CMB
  //
  // PRT-A-SEP-NEXT:        {{^ *}}#pragma acc parallel num_gangs(2){{$}}
  // PRT-AO-SEP-NEXT:       {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  // PRT-A-SEP-NEXT:        {
  // PRT-A-SEP-NEXT:          {{^ *}}#pragma acc loop [[ACCC0]]
  // PRT-AO-SEP-OSEQ0-SAME:   {{^}} // discarded in OpenMP translation
  // PRT-A-SEP-SAME:          {{^$}}
  // PRT-AO-SEP-OPRG0-NEXT:   {{^ *}}// #pragma omp [[OMPDP0]]{{$}}
  //
  // PRT-O-SEP-NEXT:        {{^ *}}#pragma omp target teams num_teams(2){{$}}
  // PRT-OA-SEP-NEXT:       {{^ *}}// #pragma acc parallel num_gangs(2){{$}}
  // PRT-O-SEP-NEXT:        {
  // PRT-O-SEP-OPRG0-NEXT:    {{^ *}}#pragma omp [[OMPDP0]]{{$}}
  // PRT-OA-SEP-NEXT:         {{^ *}}// #pragma acc loop [[ACCC0]]
  // PRT-OA-SEP-OSEQ0-SAME:   {{^}} // discarded in OpenMP translation
  // PRT-OA-SEP-SAME:         {{^$}}
  //
  // PRT-PRE-SEP-NEXT: #else
  // PRT-PRE-SEP-NEXT:      {{^ *}}#pragma acc parallel loop num_gangs(2) ACCC0{{$}}
  // PRT-PRE-SEP-NEXT: #endif
  //
  //
  // PRT-PRE-CMB-NEXT: #if !CMB
  //
  // PRT-PRE-CMB-NEXT:      {{^ *}}#pragma acc parallel num_gangs(2)
  // PRT-PRE-CMB-NEXT:      {
  // PRT-PRE-CMB-NEXT:        {{^ *}}#pragma acc loop ACCC0
  //
  // PRT-PRE-CMB-NEXT: #else
  //
  // PRT-A-CMB-NEXT:        {{^ *}}#pragma acc parallel loop num_gangs(2) [[ACCC0]]{{$}}
  // PRT-AO-CMB-NEXT:       {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  // PRT-AO-CMB-OPRG0-NEXT: {{^ *}}// #pragma omp [[OMPDP0]]{{$}}
  //
  // PRT-O-CMB-NEXT:        {{^ *}}#pragma omp target teams num_teams(2){{$}}
  // PRT-O-CMB-OPRG0-NEXT:  {{^ *}}#pragma omp [[OMPDP0]]{{$}}
  // PRT-OA-CMB-NEXT:       {{^ *}}// #pragma acc parallel loop num_gangs(2) [[ACCC0]]{{$}}
  //
  // PRT-PRE-CMB-NEXT: #endif
  //
  //
  // PRT-NOACC:             {
#if !CMB
  #pragma acc parallel num_gangs(2)
  {
    #pragma acc loop ACCC0
#else
    #pragma acc parallel loop num_gangs(2) ACCC0
#endif
    // PRT-NEXT: for (int i = 0; i < {{.*}}; ++i) {
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
      // PRT-A-NEXT:              {{^ *}}#pragma acc loop [[ACCC1]]
      // PRT-AO-OSEQ1-SAME:       {{^}} // discarded in OpenMP translation
      // PRT-A-SAME:              {{^$}}
      // PRT-AO-OPRG1-OSIMP1-NEXT: {{^ *}}// #pragma omp [[OMPDP1]]{{$}}
      // PRT-AO-OPRG1-OSEXP1-NEXT: {{^ *}}// #pragma omp [[OMPDP1]] shared(i){{$}}
      //
      // PRT-O-OPRG1-OSIMP1-NEXT: {{^ *}}#pragma omp [[OMPDP1]]{{$}}
      // PRT-O-OPRG1-OSEXP1-NEXT: {{^ *}}#pragma omp [[OMPDP1]] shared(i){{$}}
      // PRT-OA-NEXT:              {{^ *}}// #pragma acc loop [[ACCC1]]
      // PRT-OA-OSEQ1-SAME:        {{^}} // discarded in OpenMP translation
      // PRT-OA-SAME:              {{^$}}
      //
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
        // PRT-A-NEXT:              {{^ *}}#pragma acc loop [[ACCC2]]
        // PRT-AO-OSEQ2-SAME:       {{^}} // discarded in OpenMP translation
        // PRT-A-SAME:              {{^$}}
        // PRT-AO-OPRG2-OSIMP2-NEXT: {{^ *}}// #pragma omp [[OMPDP2]]{{$}}
        // PRT-AO-OPRG2-OSEXP2-NEXT: {{^ *}}// #pragma omp [[OMPDP2]] shared(i,j){{$}}
        //
        // PRT-O-OPRG2-OSIMP2-NEXT: {{^ *}}#pragma omp [[OMPDP2]]{{$}}
        // PRT-O-OPRG2-OSEXP2-NEXT: {{^ *}}#pragma omp [[OMPDP2]] shared(i,j){{$}}
        // PRT-OA-NEXT:              {{^ *}}// #pragma acc loop [[ACCC2]]
        // PRT-OA-OSEQ2-SAME:        {{^}} // discarded in OpenMP translation
        // PRT-OA-SAME:              {{^$}}
        //
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
        } // PRT-NEXT: }
      } // PRT-NEXT: }
    } // PRT-NEXT: }
// PRT-PRE: #if !CMB
#if !CMB
  }
#endif
  // PRT-NOACC-NEXT: }
  // PRT-O-SEP-NEXT: }
  // PRT-A-SEP-NEXT: }
// PRT-PRE: #endif

  // Close off EXE*-DAGs.
  // PRT-NEXT: printf
  // EXE-NOT: {{.}}
  // EXE-NEXT: END
  printf("END\n");

  // PRT-NEXT: return 0;
  return 0;

} // PRT-NEXT: }
// EXE-NOT: {{.}}
