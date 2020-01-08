// Check implicit and explicit data attributes on "acc parallel".
//
// When ADD_LOOP_TO_PAR is not set, this file checks implicit and explicit
// data attributes on "acc parallel" without "loop".
//
// When ADD_LOOP_TO_PAR is set, it adds "loop seq" and a for loop to those "acc
// parallel" directives in order to check data attributes for combined "acc
// parallel loop" directives.
//
// RUN: %data directives {
// RUN:   (dir=PAR     dir-cflags=                 )
// RUN:   (dir=PARLOOP dir-cflags=-DADD_LOOP_TO_PAR)
// RUN: }

// Check -ast-dump before and after AST serialization.
//
// RUN: %data dmps {
// RUN:   (mode=MODE_I   pre=DMP,DMP-I,DMP-ICCiCoF,DMP-%[dir],DMP-%[dir]-I,DMP-%[dir]-ICCiCoF)
// RUN:   (mode=MODE_C0  pre=DMP,DMP-C,DMP-ICCiCoF,DMP-CCiCo,DMP-CCiCoF,DMP-CCiCoFP,DMP-%[dir],DMP-%[dir]-C,DMP-%[dir]-ICCiCoF,DMP-%[dir]-CCiCo,DMP-%[dir]-CCiCoF,DMP-%[dir]-CCiCoFP)
// RUN:   (mode=MODE_C1  pre=DMP,DMP-C,DMP-ICCiCoF,DMP-CCiCo,DMP-CCiCoF,DMP-CCiCoFP,DMP-%[dir],DMP-%[dir]-C,DMP-%[dir]-ICCiCoF,DMP-%[dir]-CCiCo,DMP-%[dir]-CCiCoF,DMP-%[dir]-CCiCoFP)
// RUN:   (mode=MODE_C2  pre=DMP,DMP-C,DMP-ICCiCoF,DMP-CCiCo,DMP-CCiCoF,DMP-CCiCoFP,DMP-%[dir],DMP-%[dir]-C,DMP-%[dir]-ICCiCoF,DMP-%[dir]-CCiCo,DMP-%[dir]-CCiCoF,DMP-%[dir]-CCiCoFP)
// RUN:   (mode=MODE_Ci0 pre=DMP,DMP-Ci,DMP-ICCiCoF,DMP-CCiCo,DMP-CCiCoF,DMP-CCiCoFP,DMP-%[dir],DMP-%[dir]-Ci,DMP-%[dir]-ICCiCoF,DMP-%[dir]-CCiCo,DMP-%[dir]-CCiCoF,DMP-%[dir]-CCiCoFP)
// RUN:   (mode=MODE_Ci1 pre=DMP,DMP-Ci,DMP-ICCiCoF,DMP-CCiCo,DMP-CCiCoF,DMP-CCiCoFP,DMP-%[dir],DMP-%[dir]-Ci,DMP-%[dir]-ICCiCoF,DMP-%[dir]-CCiCo,DMP-%[dir]-CCiCoF,DMP-%[dir]-CCiCoFP)
// RUN:   (mode=MODE_Ci2 pre=DMP,DMP-Ci,DMP-ICCiCoF,DMP-CCiCo,DMP-CCiCoF,DMP-CCiCoFP,DMP-%[dir],DMP-%[dir]-Ci,DMP-%[dir]-ICCiCoF,DMP-%[dir]-CCiCo,DMP-%[dir]-CCiCoF,DMP-%[dir]-CCiCoFP)
// RUN:   (mode=MODE_Co0 pre=DMP,DMP-Co,DMP-ICCiCoF,DMP-CCiCo,DMP-CCiCoF,DMP-CCiCoFP,DMP-%[dir],DMP-%[dir]-Co,DMP-%[dir]-ICCiCoF,DMP-%[dir]-CCiCo,DMP-%[dir]-CCiCoF,DMP-%[dir]-CCiCoFP)
// RUN:   (mode=MODE_Co1 pre=DMP,DMP-Co,DMP-ICCiCoF,DMP-CCiCo,DMP-CCiCoF,DMP-CCiCoFP,DMP-%[dir],DMP-%[dir]-Co,DMP-%[dir]-ICCiCoF,DMP-%[dir]-CCiCo,DMP-%[dir]-CCiCoF,DMP-%[dir]-CCiCoFP)
// RUN:   (mode=MODE_Co2 pre=DMP,DMP-Co,DMP-ICCiCoF,DMP-CCiCo,DMP-CCiCoF,DMP-CCiCoFP,DMP-%[dir],DMP-%[dir]-Co,DMP-%[dir]-ICCiCoF,DMP-%[dir]-CCiCo,DMP-%[dir]-CCiCoF,DMP-%[dir]-CCiCoFP)
// RUN:   (mode=MODE_F   pre=DMP,DMP-F,DMP-ICCiCoF,DMP-CCiCoF,DMP-CCiCoFP,DMP-%[dir],DMP-%[dir]-F,DMP-%[dir]-ICCiCoF,DMP-%[dir]-CCiCoF,DMP-%[dir]-CCiCoFP)
// RUN:   (mode=MODE_P   pre=DMP,DMP-P,DMP-CCiCoFP,DMP-%[dir],DMP-%[dir]-P,DMP-%[dir]-CCiCoFP)
// RUN: }
// RUN: %for directives {
// RUN:   %for dmps {
// RUN:     %clang -Xclang -verify -Xclang -ast-dump -fsyntax-only -fopenacc \
// RUN:            -DMODE=%[mode] %[dir-cflags] %s \
// RUN:     | FileCheck -check-prefixes=%[pre] %s
// RUN:     %clang -Xclang -verify -fopenacc -emit-ast -o %t.ast \
// RUN:            -DMODE=%[mode] %[dir-cflags] %s
// RUN:     %clang_cc1 -ast-dump-all %t.ast \
// RUN:     | FileCheck -check-prefixes=%[pre] %s
// RUN:   }
// RUN: }

// Check -ast-print after AST serialization.  -ast-print before serialization
// is indirectly checked below via -fopenacc[-ast]-print.
//
// RUN: %data asts {
// RUN:   (mode=MODE_I   pre=PRT,PRT-I,PRT-%[dir],PRT-%[dir]-I,PRT-A,PRT-A-I,PRT-A-%[dir],PRT-A-%[dir]-I)
// RUN:   (mode=MODE_C0  pre=PRT,PRT-C0,PRT-%[dir],PRT-%[dir]-C0,PRT-A,PRT-A-C0,PRT-A-%[dir],PRT-A-%[dir]-C0)
// RUN:   (mode=MODE_C1  pre=PRT,PRT-C1,PRT-%[dir],PRT-%[dir]-C1,PRT-A,PRT-A-C1,PRT-A-%[dir],PRT-A-%[dir]-C1)
// RUN:   (mode=MODE_C2  pre=PRT,PRT-C2,PRT-%[dir],PRT-%[dir]-C2,PRT-A,PRT-A-C2,PRT-A-%[dir],PRT-A-%[dir]-C2)
// RUN:   (mode=MODE_Ci0 pre=PRT,PRT-Ci0,PRT-%[dir],PRT-%[dir]-Ci0,PRT-A,PRT-A-Ci0,PRT-A-%[dir],PRT-A-%[dir]-Ci0)
// RUN:   (mode=MODE_Ci1 pre=PRT,PRT-Ci1,PRT-%[dir],PRT-%[dir]-Ci1,PRT-A,PRT-A-Ci1,PRT-A-%[dir],PRT-A-%[dir]-Ci1)
// RUN:   (mode=MODE_Ci2 pre=PRT,PRT-Ci2,PRT-%[dir],PRT-%[dir]-Ci2,PRT-A,PRT-A-Ci2,PRT-A-%[dir],PRT-A-%[dir]-Ci2)
// RUN:   (mode=MODE_Co0 pre=PRT,PRT-Co0,PRT-%[dir],PRT-%[dir]-Co0,PRT-A,PRT-A-Co0,PRT-A-%[dir],PRT-A-%[dir]-Co0)
// RUN:   (mode=MODE_Co1 pre=PRT,PRT-Co1,PRT-%[dir],PRT-%[dir]-Co1,PRT-A,PRT-A-Co1,PRT-A-%[dir],PRT-A-%[dir]-Co1)
// RUN:   (mode=MODE_Co2 pre=PRT,PRT-Co2,PRT-%[dir],PRT-%[dir]-Co2,PRT-A,PRT-A-Co2,PRT-A-%[dir],PRT-A-%[dir]-Co2)
// RUN:   (mode=MODE_F   pre=PRT,PRT-F,PRT-%[dir],PRT-%[dir]-F,PRT-A,PRT-A-F,PRT-A-%[dir],PRT-A-%[dir]-F)
// RUN:   (mode=MODE_P   pre=PRT,PRT-P,PRT-%[dir],PRT-%[dir]-P,PRT-A,PRT-A-P,PRT-A-%[dir],PRT-A-%[dir]-P)
// RUN: }
// RUN: %for directives {
// RUN:   %for asts {
// RUN:     %clang -Xclang -verify -fopenacc -emit-ast -o %t.ast \
// RUN:            -DMODE=%[mode] %[dir-cflags] %s
// RUN:     %clang_cc1 -ast-print %t.ast 2>&1 \
// RUN:     | FileCheck -check-prefixes=%[pre] %s
// RUN:   }
// RUN: }

// Check -fopenacc[-ast]-print.  Default is checked above.
//
// Strip comments and blank lines so checking -fopenacc-print output is easier.
// RUN: echo "// expected""-no-diagnostics" > %t-acc.c
// RUN: grep -v '^ *\(//.*\)\?$' %s | sed 's,//.*,,' >> %t-acc.c
//
// TODO: If lit were to support %for inside a %data, we could iterate prt-opts
// (which would need additional fields) within prt-args, significantly
// shortening the prt-args definition.
//
// RUN: %data prt-opts {
// RUN:   (prt-opt=-fopenacc-ast-print)
// RUN:   (prt-opt=-fopenacc-print    )
// RUN: }
// RUN: %data prt-args {
// RUN:   (mode=MODE_I  prt=%[prt-opt]=acc     prt-chk=PRT,PRT-I,PRT-%[dir],PRT-%[dir]-I,PRT-A,PRT-A-I,PRT-A-%[dir],PRT-A-%[dir]-I                                              )
// RUN:   (mode=MODE_I  prt=%[prt-opt]=omp     prt-chk=PRT,PRT-I,PRT-%[dir],PRT-%[dir]-I,PRT-O,PRT-O-I,PRT-O-%[dir],PRT-O-%[dir]-I                                              )
// RUN:   (mode=MODE_I  prt=%[prt-opt]=acc-omp prt-chk=PRT,PRT-I,PRT-%[dir],PRT-%[dir]-I,PRT-A,PRT-A-I,PRT-A-%[dir],PRT-A-%[dir]-I,PRT-AO,PRT-AO-I,PRT-AO-%[dir],PRT-AO-%[dir]-I)
// RUN:   (mode=MODE_I  prt=%[prt-opt]=omp-acc prt-chk=PRT,PRT-I,PRT-%[dir],PRT-%[dir]-I,PRT-O,PRT-O-I,PRT-O-%[dir],PRT-O-%[dir]-I,PRT-OA,PRT-OA-I,PRT-OA-%[dir],PRT-OA-%[dir]-I)
//
// RUN:   (mode=MODE_C0  prt=%[prt-opt]=acc     prt-chk=PRT,PRT-C0,PRT-%[dir],PRT-%[dir]-C0,PRT-A,PRT-A-C0,PRT-A-%[dir],PRT-A-%[dir]-C0                                                )
// RUN:   (mode=MODE_C0  prt=%[prt-opt]=omp     prt-chk=PRT,PRT-C0,PRT-%[dir],PRT-%[dir]-C0,PRT-O,PRT-O-C0,PRT-O-%[dir],PRT-O-%[dir]-C0                                                )
// RUN:   (mode=MODE_C0  prt=%[prt-opt]=acc-omp prt-chk=PRT,PRT-C0,PRT-%[dir],PRT-%[dir]-C0,PRT-A,PRT-A-C0,PRT-A-%[dir],PRT-A-%[dir]-C0,PRT-AO,PRT-AO-C0,PRT-AO-%[dir],PRT-AO-%[dir]-C0)
// RUN:   (mode=MODE_C0  prt=%[prt-opt]=omp-acc prt-chk=PRT,PRT-C0,PRT-%[dir],PRT-%[dir]-C0,PRT-O,PRT-O-C0,PRT-O-%[dir],PRT-O-%[dir]-C0,PRT-OA,PRT-OA-C0,PRT-OA-%[dir],PRT-OA-%[dir]-C0)
//
// RUN:   (mode=MODE_C1  prt=%[prt-opt]=acc     prt-chk=PRT,PRT-C1,PRT-%[dir],PRT-%[dir]-C1,PRT-A,PRT-A-C1,PRT-A-%[dir],PRT-A-%[dir]-C1                                                )
// RUN:   (mode=MODE_C1  prt=%[prt-opt]=omp     prt-chk=PRT,PRT-C1,PRT-%[dir],PRT-%[dir]-C1,PRT-O,PRT-O-C1,PRT-O-%[dir],PRT-O-%[dir]-C1                                                )
// RUN:   (mode=MODE_C1  prt=%[prt-opt]=acc-omp prt-chk=PRT,PRT-C1,PRT-%[dir],PRT-%[dir]-C1,PRT-A,PRT-A-C1,PRT-A-%[dir],PRT-A-%[dir]-C1,PRT-AO,PRT-AO-C1,PRT-AO-%[dir],PRT-AO-%[dir]-C1)
// RUN:   (mode=MODE_C1  prt=%[prt-opt]=omp-acc prt-chk=PRT,PRT-C1,PRT-%[dir],PRT-%[dir]-C1,PRT-O,PRT-O-C1,PRT-O-%[dir],PRT-O-%[dir]-C1,PRT-OA,PRT-OA-C1,PRT-OA-%[dir],PRT-OA-%[dir]-C1)
//
// RUN:   (mode=MODE_C2  prt=%[prt-opt]=acc     prt-chk=PRT,PRT-C2,PRT-%[dir],PRT-%[dir]-C2,PRT-A,PRT-A-C2,PRT-A-%[dir],PRT-A-%[dir]-C2                                                )
// RUN:   (mode=MODE_C2  prt=%[prt-opt]=omp     prt-chk=PRT,PRT-C2,PRT-%[dir],PRT-%[dir]-C2,PRT-O,PRT-O-C2,PRT-O-%[dir],PRT-O-%[dir]-C2                                                )
// RUN:   (mode=MODE_C2  prt=%[prt-opt]=acc-omp prt-chk=PRT,PRT-C2,PRT-%[dir],PRT-%[dir]-C2,PRT-A,PRT-A-C2,PRT-A-%[dir],PRT-A-%[dir]-C2,PRT-AO,PRT-AO-C2,PRT-AO-%[dir],PRT-AO-%[dir]-C2)
// RUN:   (mode=MODE_C2  prt=%[prt-opt]=omp-acc prt-chk=PRT,PRT-C2,PRT-%[dir],PRT-%[dir]-C2,PRT-O,PRT-O-C2,PRT-O-%[dir],PRT-O-%[dir]-C2,PRT-OA,PRT-OA-C2,PRT-OA-%[dir],PRT-OA-%[dir]-C2)
//
// RUN:   (mode=MODE_Ci0 prt=%[prt-opt]=acc     prt-chk=PRT,PRT-Ci0,PRT-%[dir],PRT-%[dir]-Ci0,PRT-A,PRT-A-Ci0,PRT-A-%[dir],PRT-A-%[dir]-Ci0                                                  )
// RUN:   (mode=MODE_Ci0 prt=%[prt-opt]=omp     prt-chk=PRT,PRT-Ci0,PRT-%[dir],PRT-%[dir]-Ci0,PRT-O,PRT-O-Ci0,PRT-O-%[dir],PRT-O-%[dir]-Ci0                                                  )
// RUN:   (mode=MODE_Ci0 prt=%[prt-opt]=acc-omp prt-chk=PRT,PRT-Ci0,PRT-%[dir],PRT-%[dir]-Ci0,PRT-A,PRT-A-Ci0,PRT-A-%[dir],PRT-A-%[dir]-Ci0,PRT-AO,PRT-AO-Ci0,PRT-AO-%[dir],PRT-AO-%[dir]-Ci0)
// RUN:   (mode=MODE_Ci0 prt=%[prt-opt]=omp-acc prt-chk=PRT,PRT-Ci0,PRT-%[dir],PRT-%[dir]-Ci0,PRT-O,PRT-O-Ci0,PRT-O-%[dir],PRT-O-%[dir]-Ci0,PRT-OA,PRT-OA-Ci0,PRT-OA-%[dir],PRT-OA-%[dir]-Ci0)
//
// RUN:   (mode=MODE_Ci1 prt=%[prt-opt]=acc     prt-chk=PRT,PRT-Ci1,PRT-%[dir],PRT-%[dir]-Ci1,PRT-A,PRT-A-Ci1,PRT-A-%[dir],PRT-A-%[dir]-Ci1                                                  )
// RUN:   (mode=MODE_Ci1 prt=%[prt-opt]=omp     prt-chk=PRT,PRT-Ci1,PRT-%[dir],PRT-%[dir]-Ci1,PRT-O,PRT-O-Ci1,PRT-O-%[dir],PRT-O-%[dir]-Ci1                                                  )
// RUN:   (mode=MODE_Ci1 prt=%[prt-opt]=acc-omp prt-chk=PRT,PRT-Ci1,PRT-%[dir],PRT-%[dir]-Ci1,PRT-A,PRT-A-Ci1,PRT-A-%[dir],PRT-A-%[dir]-Ci1,PRT-AO,PRT-AO-Ci1,PRT-AO-%[dir],PRT-AO-%[dir]-Ci1)
// RUN:   (mode=MODE_Ci1 prt=%[prt-opt]=omp-acc prt-chk=PRT,PRT-Ci1,PRT-%[dir],PRT-%[dir]-Ci1,PRT-O,PRT-O-Ci1,PRT-O-%[dir],PRT-O-%[dir]-Ci1,PRT-OA,PRT-OA-Ci1,PRT-OA-%[dir],PRT-OA-%[dir]-Ci1)
//
// RUN:   (mode=MODE_Ci2 prt=%[prt-opt]=acc     prt-chk=PRT,PRT-Ci2,PRT-%[dir],PRT-%[dir]-Ci2,PRT-A,PRT-A-Ci2,PRT-A-%[dir],PRT-A-%[dir]-Ci2                                                  )
// RUN:   (mode=MODE_Ci2 prt=%[prt-opt]=omp     prt-chk=PRT,PRT-Ci2,PRT-%[dir],PRT-%[dir]-Ci2,PRT-O,PRT-O-Ci2,PRT-O-%[dir],PRT-O-%[dir]-Ci2                                                  )
// RUN:   (mode=MODE_Ci2 prt=%[prt-opt]=acc-omp prt-chk=PRT,PRT-Ci2,PRT-%[dir],PRT-%[dir]-Ci2,PRT-A,PRT-A-Ci2,PRT-A-%[dir],PRT-A-%[dir]-Ci2,PRT-AO,PRT-AO-Ci2,PRT-AO-%[dir],PRT-AO-%[dir]-Ci2)
// RUN:   (mode=MODE_Ci2 prt=%[prt-opt]=omp-acc prt-chk=PRT,PRT-Ci2,PRT-%[dir],PRT-%[dir]-Ci2,PRT-O,PRT-O-Ci2,PRT-O-%[dir],PRT-O-%[dir]-Ci2,PRT-OA,PRT-OA-Ci2,PRT-OA-%[dir],PRT-OA-%[dir]-Ci2)
//
// RUN:   (mode=MODE_Co0 prt=%[prt-opt]=acc     prt-chk=PRT,PRT-Co0,PRT-%[dir],PRT-%[dir]-Co0,PRT-A,PRT-A-Co0,PRT-A-%[dir],PRT-A-%[dir]-Co0                                                  )
// RUN:   (mode=MODE_Co0 prt=%[prt-opt]=omp     prt-chk=PRT,PRT-Co0,PRT-%[dir],PRT-%[dir]-Co0,PRT-O,PRT-O-Co0,PRT-O-%[dir],PRT-O-%[dir]-Co0                                                  )
// RUN:   (mode=MODE_Co0 prt=%[prt-opt]=acc-omp prt-chk=PRT,PRT-Co0,PRT-%[dir],PRT-%[dir]-Co0,PRT-A,PRT-A-Co0,PRT-A-%[dir],PRT-A-%[dir]-Co0,PRT-AO,PRT-AO-Co0,PRT-AO-%[dir],PRT-AO-%[dir]-Co0)
// RUN:   (mode=MODE_Co0 prt=%[prt-opt]=omp-acc prt-chk=PRT,PRT-Co0,PRT-%[dir],PRT-%[dir]-Co0,PRT-O,PRT-O-Co0,PRT-O-%[dir],PRT-O-%[dir]-Co0,PRT-OA,PRT-OA-Co0,PRT-OA-%[dir],PRT-OA-%[dir]-Co0)
//
// RUN:   (mode=MODE_Co1 prt=%[prt-opt]=acc     prt-chk=PRT,PRT-Co1,PRT-%[dir],PRT-%[dir]-Co1,PRT-A,PRT-A-Co1,PRT-A-%[dir],PRT-A-%[dir]-Co1                                                  )
// RUN:   (mode=MODE_Co1 prt=%[prt-opt]=omp     prt-chk=PRT,PRT-Co1,PRT-%[dir],PRT-%[dir]-Co1,PRT-O,PRT-O-Co1,PRT-O-%[dir],PRT-O-%[dir]-Co1                                                  )
// RUN:   (mode=MODE_Co1 prt=%[prt-opt]=acc-omp prt-chk=PRT,PRT-Co1,PRT-%[dir],PRT-%[dir]-Co1,PRT-A,PRT-A-Co1,PRT-A-%[dir],PRT-A-%[dir]-Co1,PRT-AO,PRT-AO-Co1,PRT-AO-%[dir],PRT-AO-%[dir]-Co1)
// RUN:   (mode=MODE_Co1 prt=%[prt-opt]=omp-acc prt-chk=PRT,PRT-Co1,PRT-%[dir],PRT-%[dir]-Co1,PRT-O,PRT-O-Co1,PRT-O-%[dir],PRT-O-%[dir]-Co1,PRT-OA,PRT-OA-Co1,PRT-OA-%[dir],PRT-OA-%[dir]-Co1)
//
// RUN:   (mode=MODE_Co2 prt=%[prt-opt]=acc     prt-chk=PRT,PRT-Co2,PRT-%[dir],PRT-%[dir]-Co2,PRT-A,PRT-A-Co2,PRT-A-%[dir],PRT-A-%[dir]-Co2                                                  )
// RUN:   (mode=MODE_Co2 prt=%[prt-opt]=omp     prt-chk=PRT,PRT-Co2,PRT-%[dir],PRT-%[dir]-Co2,PRT-O,PRT-O-Co2,PRT-O-%[dir],PRT-O-%[dir]-Co2                                                  )
// RUN:   (mode=MODE_Co2 prt=%[prt-opt]=acc-omp prt-chk=PRT,PRT-Co2,PRT-%[dir],PRT-%[dir]-Co2,PRT-A,PRT-A-Co2,PRT-A-%[dir],PRT-A-%[dir]-Co2,PRT-AO,PRT-AO-Co2,PRT-AO-%[dir],PRT-AO-%[dir]-Co2)
// RUN:   (mode=MODE_Co2 prt=%[prt-opt]=omp-acc prt-chk=PRT,PRT-Co2,PRT-%[dir],PRT-%[dir]-Co2,PRT-O,PRT-O-Co2,PRT-O-%[dir],PRT-O-%[dir]-Co2,PRT-OA,PRT-OA-Co2,PRT-OA-%[dir],PRT-OA-%[dir]-Co2)
//
// RUN:   (mode=MODE_F  prt=%[prt-opt]=acc     prt-chk=PRT,PRT-F,PRT-%[dir],PRT-%[dir]-F,PRT-A,PRT-A-F,PRT-A-%[dir],PRT-A-%[dir]-F                                              )
// RUN:   (mode=MODE_F  prt=%[prt-opt]=omp     prt-chk=PRT,PRT-F,PRT-%[dir],PRT-%[dir]-F,PRT-O,PRT-O-F,PRT-O-%[dir],PRT-O-%[dir]-F                                              )
// RUN:   (mode=MODE_F  prt=%[prt-opt]=acc-omp prt-chk=PRT,PRT-F,PRT-%[dir],PRT-%[dir]-F,PRT-A,PRT-A-F,PRT-A-%[dir],PRT-A-%[dir]-F,PRT-AO,PRT-AO-F,PRT-AO-%[dir],PRT-AO-%[dir]-F)
// RUN:   (mode=MODE_F  prt=%[prt-opt]=omp-acc prt-chk=PRT,PRT-F,PRT-%[dir],PRT-%[dir]-F,PRT-O,PRT-O-F,PRT-O-%[dir],PRT-O-%[dir]-F,PRT-OA,PRT-OA-F,PRT-OA-%[dir],PRT-OA-%[dir]-F)
//
// RUN:   (mode=MODE_P  prt=%[prt-opt]=acc     prt-chk=PRT,PRT-P,PRT-%[dir],PRT-%[dir]-P,PRT-A,PRT-A-P,PRT-A-%[dir],PRT-A-%[dir]-P                                              )
// RUN:   (mode=MODE_P  prt=%[prt-opt]=omp     prt-chk=PRT,PRT-P,PRT-%[dir],PRT-%[dir]-P,PRT-O,PRT-O-P,PRT-O-%[dir],PRT-O-%[dir]-P                                              )
// RUN:   (mode=MODE_P  prt=%[prt-opt]=acc-omp prt-chk=PRT,PRT-P,PRT-%[dir],PRT-%[dir]-P,PRT-A,PRT-A-P,PRT-A-%[dir],PRT-A-%[dir]-P,PRT-AO,PRT-AO-P,PRT-AO-%[dir],PRT-AO-%[dir]-P)
// RUN:   (mode=MODE_P  prt=%[prt-opt]=omp-acc prt-chk=PRT,PRT-P,PRT-%[dir],PRT-%[dir]-P,PRT-O,PRT-O-P,PRT-O-%[dir],PRT-O-%[dir]-P,PRT-OA,PRT-OA-P,PRT-OA-%[dir],PRT-OA-%[dir]-P)
// RUN: }
// RUN: %for directives {
// RUN:   %for prt-opts {
// RUN:     %for prt-args {
// RUN:       %clang -Xclang -verify %[prt] -DMODE=%[mode] %[dir-cflags] \
// RUN:              %t-acc.c \
// RUN:       | FileCheck -check-prefixes=%[prt-chk] %s
// RUN:     }
// RUN:   }
// RUN: }

// Can we print the OpenMP source code, compile, and run it successfully?
//
// We targeting the host, memory is shared, so copy, copyin, copyout, and their
// aliases all act like copy.
// RUN: %data exes {
// RUN:   (mode=MODE_I   cflags=                 pre-host=EXE-I,EXE-ICCi,EXE-ICCiF,EXE-ICCiFP,EXE-ICCo,EXE-ICiFP,EXE-IF,EXE pre-tgt=EXE-I,EXE-ICCi,EXE-ICCiF,EXE-ICCiFP,EXE-ICCo,EXE-ICiFP,EXE-IF,EXE  )
// RUN:   (mode=MODE_I   cflags=-DSTORAGE=static pre-host=EXE-I,EXE-ICCi,EXE-ICCiF,EXE-ICCiFP,EXE-ICCo,EXE-ICiFP,EXE-IF,EXE pre-tgt=EXE-I,EXE-ICCi,EXE-ICCiF,EXE-ICCiFP,EXE-ICCo,EXE-ICiFP,EXE-IF,EXE  )
// RUN:   (mode=MODE_C0  cflags=                 pre-host=EXE-C,EXE-ICCi,EXE-ICCiF,EXE-ICCiFP,EXE-ICCo,EXE-CCi,EXE-CCo,EXE  pre-tgt=EXE-C,EXE-ICCi,EXE-ICCiF,EXE-ICCiFP,EXE-ICCo,EXE-CCi,EXE-CCo,EXE   )
// RUN:   (mode=MODE_C0  cflags=-DSTORAGE=static pre-host=EXE-C,EXE-ICCi,EXE-ICCiF,EXE-ICCiFP,EXE-ICCo,EXE-CCi,EXE-CCo,EXE  pre-tgt=EXE-C,EXE-ICCi,EXE-ICCiF,EXE-ICCiFP,EXE-ICCo,EXE-CCi,EXE-CCo,EXE   )
// RUN:   (mode=MODE_C1  cflags=                 pre-host=EXE-C,EXE-ICCi,EXE-ICCiF,EXE-ICCiFP,EXE-ICCo,EXE-CCi,EXE-CCo,EXE  pre-tgt=EXE-C,EXE-ICCi,EXE-ICCiF,EXE-ICCiFP,EXE-ICCo,EXE-CCi,EXE-CCo,EXE   )
// RUN:   (mode=MODE_C1  cflags=-DSTORAGE=static pre-host=EXE-C,EXE-ICCi,EXE-ICCiF,EXE-ICCiFP,EXE-ICCo,EXE-CCi,EXE-CCo,EXE  pre-tgt=EXE-C,EXE-ICCi,EXE-ICCiF,EXE-ICCiFP,EXE-ICCo,EXE-CCi,EXE-CCo,EXE   )
// RUN:   (mode=MODE_C2  cflags=                 pre-host=EXE-C,EXE-ICCi,EXE-ICCiF,EXE-ICCiFP,EXE-ICCo,EXE-CCi,EXE-CCo,EXE  pre-tgt=EXE-C,EXE-ICCi,EXE-ICCiF,EXE-ICCiFP,EXE-ICCo,EXE-CCi,EXE-CCo,EXE   )
// RUN:   (mode=MODE_C2  cflags=-DSTORAGE=static pre-host=EXE-C,EXE-ICCi,EXE-ICCiF,EXE-ICCiFP,EXE-ICCo,EXE-CCi,EXE-CCo,EXE  pre-tgt=EXE-C,EXE-ICCi,EXE-ICCiF,EXE-ICCiFP,EXE-ICCo,EXE-CCi,EXE-CCo,EXE   )
// RUN:   (mode=MODE_Ci0 cflags=                 pre-host=EXE-C,EXE-ICCi,EXE-ICCiF,EXE-ICCiFP,EXE-ICCo,EXE-CCi,EXE-CCo,EXE  pre-tgt=EXE-Ci,EXE-ICCi,EXE-ICCiF,EXE-ICCiFP,EXE-ICiFP,EXE-CCi,EXE-CiFP,EXE)
// RUN:   (mode=MODE_Ci0 cflags=-DSTORAGE=static pre-host=EXE-C,EXE-ICCi,EXE-ICCiF,EXE-ICCiFP,EXE-ICCo,EXE-CCi,EXE-CCo,EXE  pre-tgt=EXE-Ci,EXE-ICCi,EXE-ICCiF,EXE-ICCiFP,EXE-ICiFP,EXE-CCi,EXE-CiFP,EXE)
// RUN:   (mode=MODE_Ci1 cflags=                 pre-host=EXE-C,EXE-ICCi,EXE-ICCiF,EXE-ICCiFP,EXE-ICCo,EXE-CCi,EXE-CCo,EXE  pre-tgt=EXE-Ci,EXE-ICCi,EXE-ICCiF,EXE-ICCiFP,EXE-ICiFP,EXE-CCi,EXE-CiFP,EXE)
// RUN:   (mode=MODE_Ci1 cflags=-DSTORAGE=static pre-host=EXE-C,EXE-ICCi,EXE-ICCiF,EXE-ICCiFP,EXE-ICCo,EXE-CCi,EXE-CCo,EXE  pre-tgt=EXE-Ci,EXE-ICCi,EXE-ICCiF,EXE-ICCiFP,EXE-ICiFP,EXE-CCi,EXE-CiFP,EXE)
// RUN:   (mode=MODE_Ci2 cflags=                 pre-host=EXE-C,EXE-ICCi,EXE-ICCiF,EXE-ICCiFP,EXE-ICCo,EXE-CCi,EXE-CCo,EXE  pre-tgt=EXE-Ci,EXE-ICCi,EXE-ICCiF,EXE-ICCiFP,EXE-ICiFP,EXE-CCi,EXE-CiFP,EXE)
// RUN:   (mode=MODE_Ci2 cflags=-DSTORAGE=static pre-host=EXE-C,EXE-ICCi,EXE-ICCiF,EXE-ICCiFP,EXE-ICCo,EXE-CCi,EXE-CCo,EXE  pre-tgt=EXE-Ci,EXE-ICCi,EXE-ICCiF,EXE-ICCiFP,EXE-ICiFP,EXE-CCi,EXE-CiFP,EXE)
// RUN:   (mode=MODE_Co0 cflags=                 pre-host=EXE-C,EXE-ICCi,EXE-ICCiF,EXE-ICCiFP,EXE-ICCo,EXE-CCi,EXE-CCo,EXE  pre-tgt=EXE-Co,EXE-ICCo,EXE-CCo,EXE-CoP,EXE                                )
// RUN:   (mode=MODE_Co0 cflags=-DSTORAGE=static pre-host=EXE-C,EXE-ICCi,EXE-ICCiF,EXE-ICCiFP,EXE-ICCo,EXE-CCi,EXE-CCo,EXE  pre-tgt=EXE-Co,EXE-ICCo,EXE-CCo,EXE-CoP,EXE                                )
// RUN:   (mode=MODE_Co1 cflags=                 pre-host=EXE-C,EXE-ICCi,EXE-ICCiF,EXE-ICCiFP,EXE-ICCo,EXE-CCi,EXE-CCo,EXE  pre-tgt=EXE-Co,EXE-ICCo,EXE-CCo,EXE-CoP,EXE                                )
// RUN:   (mode=MODE_Co1 cflags=-DSTORAGE=static pre-host=EXE-C,EXE-ICCi,EXE-ICCiF,EXE-ICCiFP,EXE-ICCo,EXE-CCi,EXE-CCo,EXE  pre-tgt=EXE-Co,EXE-ICCo,EXE-CCo,EXE-CoP,EXE                                )
// RUN:   (mode=MODE_Co2 cflags=                 pre-host=EXE-C,EXE-ICCi,EXE-ICCiF,EXE-ICCiFP,EXE-ICCo,EXE-CCi,EXE-CCo,EXE  pre-tgt=EXE-Co,EXE-ICCo,EXE-CCo,EXE-CoP,EXE                                )
// RUN:   (mode=MODE_Co2 cflags=-DSTORAGE=static pre-host=EXE-C,EXE-ICCi,EXE-ICCiF,EXE-ICCiFP,EXE-ICCo,EXE-CCi,EXE-CCo,EXE  pre-tgt=EXE-Co,EXE-ICCo,EXE-CCo,EXE-CoP,EXE                                )
// RUN:   (mode=MODE_F   cflags=                 pre-host=EXE-F,EXE-ICCiF,EXE-ICCiFP,EXE-ICiFP,EXE-CiFP,EXE-IF,EXE          pre-tgt=EXE-F,EXE-ICCiF,EXE-ICCiFP,EXE-ICiFP,EXE-CiFP,EXE-IF,EXE           )
// RUN:   (mode=MODE_F   cflags=-DSTORAGE=static pre-host=EXE-F,EXE-ICCiF,EXE-ICCiFP,EXE-ICiFP,EXE-CiFP,EXE-IF,EXE          pre-tgt=EXE-F,EXE-ICCiF,EXE-ICCiFP,EXE-ICiFP,EXE-CiFP,EXE-IF,EXE           )
// RUN:   (mode=MODE_P   cflags=                 pre-host=EXE-P,EXE-CoP,EXE-ICCiFP,EXE-ICiFP,EXE-CiFP,EXE                   pre-tgt=EXE-P,EXE-ICCiFP,EXE-ICiFP,EXE-CiFP,EXE                            )
// RUN:   (mode=MODE_P   cflags=-DSTORAGE=static pre-host=EXE-P,EXE-CoP,EXE-ICCiFP,EXE-ICiFP,EXE-CiFP,EXE                   pre-tgt=EXE-P,EXE-ICCiFP,EXE-ICiFP,EXE-CiFP,EXE                            )
// RUN: }
// RUN: %for directives {
// RUN:   %for exes {
// RUN:     %for prt-opts {
// RUN:       %clang -Xclang -verify %[prt-opt]=omp %s > %t-omp.c \
// RUN:              -DMODE=%[mode] %[cflags] %[dir-cflags]
// RUN:       echo "// expected""-no-diagnostics" >> %t-omp.c
// RUN:       %clang -Xclang -verify -fopenmp %fopenmp-version -o %t %t-omp.c \
// RUN:              -DMODE=%[mode] %[cflags] %[dir-cflags]
// RUN:       %t | FileCheck -check-prefixes=%[pre-host] %s
// RUN:     }
// RUN:   }
// RUN: }
//
// Check execution with normal compilation.
//
// RUN: %data tgts {
// RUN:   (run-if=                tgt=HOST    tgt-cflags=                                     host-or-tgt=host)
// RUN:   (run-if=%run-if-x86_64  tgt=X86_64  tgt-cflags=-fopenmp-targets=%run-x86_64-triple  host-or-tgt=tgt )
// RUN:   (run-if=%run-if-nvptx64 tgt=NVPTX64 tgt-cflags=-fopenmp-targets=%run-nvptx64-triple host-or-tgt=tgt )
// RUN: }
// RUN: %for directives {
// RUN:   %for exes {
// RUN:     %for tgts {
// RUN:       %[run-if] %clang -Xclang -verify -fopenacc %s -o %t \
// RUN:                        -DMODE=%[mode] -DTGT_%[tgt]_EXE \
// RUN:                        %[cflags] %[tgt-cflags] %[dir-cflags]
// RUN:       %[run-if] %t > %t.out 2>&1
// RUN:       %[run-if] FileCheck -input-file %t.out %s \
// RUN:                           -check-prefixes=%[pre-%[host-or-tgt]]
// RUN:     }
// RUN:   }
// RUN: }

// Check that variables aren't passed to the outlined function in the case of
// the private clause.  However, for acc parallel loop, the private clause is
// on the effective parallel directive not the effective loop directive, so
// sometimes variables are passed to the outlined function, so don't test that
// case.
//
// RUN: %data llvm {
// RUN:   (mode=MODE_P  cflags=                )
// RUN:   (mode=MODE_P  cflags=-DSTORAGE=static)
// RUN: }
// RUN: %for llvm {
// RUN:   %clang -Xclang -verify -fopenacc -S -emit-llvm -o %t -DMODE=%[mode] \
// RUN:          %[cflags] %s
// RUN:   cat %t | FileCheck -check-prefixes=LLVM %s
// RUN: }
//
// LLVM: define internal void @.omp_outlined.
// LLVM-SAME: ({{[^,]+}} %.global_tid., {{[^,]+}} %.bound_tid.)

// END.

// expected-no-diagnostics

#include <stdio.h>
#include <stdint.h>

#if !ADD_LOOP_TO_PAR
# define LOOP
# define FORLOOP_HEAD
#else
# define LOOP loop seq
# define FORLOOP_HEAD for (int i = 0; i < 1; ++i)
#endif

#ifndef STORAGE
# define STORAGE
#endif

#define MODE_I   1
#define MODE_C0  2
#define MODE_C1  3
#define MODE_C2  4
#define MODE_Ci0 5
#define MODE_Ci1 6
#define MODE_Ci2 7
#define MODE_Co0 8
#define MODE_Co1 9
#define MODE_Co2 10
#define MODE_F   11
#define MODE_P   12

#ifndef MODE
# error MODE undefined
#elif MODE == MODE_I
# define CLAUSE(...)
#elif MODE == MODE_C0
# define CLAUSE(...) copy(__VA_ARGS__)
#elif MODE == MODE_C1
# define CLAUSE(...) pcopy(__VA_ARGS__)
#elif MODE == MODE_C2
# define CLAUSE(...) present_or_copy(__VA_ARGS__)
#elif MODE == MODE_Ci0
# define CLAUSE(...) copyin(__VA_ARGS__)
#elif MODE == MODE_Ci1
# define CLAUSE(...) pcopyin(__VA_ARGS__)
#elif MODE == MODE_Ci2
# define CLAUSE(...) present_or_copyin(__VA_ARGS__)
#elif MODE == MODE_Co0
# define CLAUSE(...) copyout(__VA_ARGS__)
#elif MODE == MODE_Co1
# define CLAUSE(...) pcopyout(__VA_ARGS__)
#elif MODE == MODE_Co2
# define CLAUSE(...) present_or_copyout(__VA_ARGS__)
#elif MODE == MODE_F
# define CLAUSE(...) firstprivate(__VA_ARGS__)
#elif MODE == MODE_P
# define CLAUSE(...) private(__VA_ARGS__)
#else
# error unknown MODE
#endif

// If __uint128_t is available, test it,  Otherwise, fake it.
#ifdef __SIZEOF_INT128__
# define MK_UINT128(Hi,Lw) ((((__uint128_t)(Hi))<<64) + (Lw))
# define DEF_UINT128(Var, Hi, Lw) __uint128_t Var = MK_UINT128(Hi,Lw)
# define DEF_UINT128_COPY(Var1, Var2) __uint128_t Var1 = Var2
# define ASSIGN_UINT128(Var, Hi, Lw) Var = MK_UINT128(Hi,Lw)
# define LIST_UINT128(Var) Var
# define HI_UINT128(Var) ((uint64_t)((Var)>>64))
# define LW_UINT128(Var) ((uint64_t)((Var)&UINT64_MAX))
#else
# define DEF_UINT128(Var, Hi, Lw) \
  uint64_t Var##_hi = Hi, Var##_lw = Lw
# define DEF_UINT128_COPY(Var1, Var2) \
  uint64_t Var1##_hi = Var2##_hi, Var1##_lw = Var2##_lw
# define ASSIGN_UINT128(Var, Hi, Lw) \
  Var##_hi = Hi; Var##_li = Lw
# define LIST_UINT128(Var) Var##_hi, Var##_lw
# define HI_UINT128(Var) Var##_hi
# define LW_UINT128(Var) Var##_lw
#endif

// Passing const variable to private isn't permitted.
#if MODE == MODE_P
# define CONST
#else
# define CONST const
#endif

// FIXME: When OpenMP offloading is activated by -fopenmp-targets, pointers
// pass into acc parallel as null, but otherwise they pass in just fine.
// What does the OpenMP spec say is supposed to happen?
#if !TGT_X86_64_EXE && !TGT_NVPTX64_EXE
# define DEREF_PTR(Ptr) (*(Ptr))
#else
# define DEREF_PTR(Ptr) 9999
#endif

// Scalar global, either:
// - implicitly/explicitly firstprivate and either:
//   - < uintptr size, captured by copy
//   - >= uintptr size, captured by ref with data copied
//   - At one time, the last case was captured by copy, and it was truncated
//     into 64 bits as a result. Thus, we exercise both the high and low 64
//     bits to check that all bits are passed through.
// - explicitly private, captured decl only
STORAGE int gi = 55;
STORAGE DEF_UINT128(gt, 0x1400, 0x59); // t=tetra integer
STORAGE const int *gp = &gi;
// Non-scalar global, either:
// - implicitly shared, no capturing
// - explicitly firstprivate, captured by ref with data copied
// - explicitly private, captured decl only
STORAGE int ga[2] = {100,200};
STORAGE struct S {int i; int j;} gs = {33, 11};
STORAGE union U {float f; int i;} gu = {.i=22};

// Unreferenced in region, if explicit firstprivate, used to crash compiler.
STORAGE int gUnref = 2;

int main() {
  // Scalar local: same as for scalar global
  STORAGE int li = 99;
  STORAGE DEF_UINT128(lt, 0x7a1, 0x62b0); // t=tetra integer
  STORAGE const int *lp = &gi;
  // Non-scalar local, either:
  // - implicitly shared, captured by ref without data copied
  // - explicitly firstprivate, captured by ref with data copied
  // - explicitly private, captured decl only
  STORAGE int la[2] = {77,88};
  STORAGE struct S ls = {222, 111};
  STORAGE union U lu = {.i=167};

  // Unreferenced in region, if explicit firstprivate, used to crash compiler.
  STORAGE int lUnref = 9;

  int giOld = gi;
  DEF_UINT128_COPY(gtOld, gt);
  const int *gpOld = gp;
  int gaOld[2] = {ga[0], ga[1]};
  int gUnrefOld = gUnref;
  struct S gsOld = gs;
  union U guOld = gu;

  int liOld = li;
  DEF_UINT128_COPY(ltOld, lt);
  const int *lpOld = lp;
  int laOld[2] = {la[0], la[1]};
  struct S lsOld = ls;
  int lUnrefOld = lUnref;
  union U luOld = lu;

  // Implicit firstprivate that is shadowed.
  STORAGE int shadowed = 111;

  // Const scalars and non-scalars should be fine with either implicit or
  // explicit data attributes.
  CONST int ci = 53;
  CONST int ca[3] = {10, 11, 12};

  // DMP-PARLOOP:             ACCParallelLoopDirective
  // DMP-PARLOOP-NEXT:          ACCSeqClause
  // DMP-PARLOOP-NEXT:          ACCNum_gangsClause
  // DMP-PARLOOP-NEXT:            IntegerLiteral {{.*}} 'int' 2
  // DMP-PARLOOP-C-NEXT:        ACCCopyClause
  // DMP-PARLOOP-Ci-NEXT:       ACCCopyinClause
  // DMP-PARLOOP-Co-NEXT:       ACCCopyoutClause
  // DMP-PARLOOP-F-NEXT:        ACCFirstprivateClause
  // DMP-PARLOOP-P-NEXT:        ACCPrivateClause
  // DMP-PARLOOP-CCiCoFP-NOT:     <implicit>
  // DMP-PARLOOP-CCiCoFP-SAME:    {{$}}
  // DMP-PARLOOP-CCiCoFP-NEXT:    DeclRefExpr {{.*}} 'gi' 'int'
  // DMP-PARLOOP-CCiCoFP-NEXT:    DeclRefExpr {{.*}} {{'gt' '__uint128_t'|'gt_hi' 'uint64_t'$[[:space:]]*DeclRefExpr .* 'gt_lw' 'uint64_t'}}
  // DMP-PARLOOP-CCiCoFP-NEXT:    DeclRefExpr {{.*}} 'gp' 'const int *'
  // DMP-PARLOOP-CCiCoFP-NEXT:    DeclRefExpr {{.*}} 'ga' 'int [2]'
  // DMP-PARLOOP-CCiCoFP-NEXT:    DeclRefExpr {{.*}} 'gs' 'struct S':'struct S'
  // DMP-PARLOOP-CCiCoFP-NEXT:    DeclRefExpr {{.*}} 'gu' 'union U':'union U'
  // DMP-PARLOOP-CCiCoFP-NEXT:    DeclRefExpr {{.*}} 'gUnref' 'int'
  // DMP-PARLOOP-C-NEXT:        ACCCopyClause
  // DMP-PARLOOP-Ci-NEXT:       ACCCopyinClause
  // DMP-PARLOOP-Co-NEXT:       ACCCopyoutClause
  // DMP-PARLOOP-F-NEXT:        ACCFirstprivateClause
  // DMP-PARLOOP-P-NEXT:        ACCPrivateClause
  // DMP-PARLOOP-CCiCoFP-NOT:     <implicit>
  // DMP-PARLOOP-CCiCoFP-SAME:    {{$}}
  // DMP-PARLOOP-CCiCoFP-NEXT:    DeclRefExpr {{.*}} 'li' 'int'
  // DMP-PARLOOP-CCiCoFP-NEXT:    DeclRefExpr {{.*}} {{'lt' '__uint128_t'|'lt_hi' 'uint64_t'$[[:space:]]*DeclRefExpr .* 'lt_lw' 'uint64_t'}}
  // DMP-PARLOOP-CCiCoFP-NEXT:    DeclRefExpr {{.*}} 'lp' 'const int *'
  // DMP-PARLOOP-CCiCoFP-NEXT:    DeclRefExpr {{.*}} 'la' 'int [2]'
  // DMP-PARLOOP-CCiCoFP-NEXT:    DeclRefExpr {{.*}} 'ls' 'struct S':'struct S'
  // DMP-PARLOOP-CCiCoFP-NEXT:    DeclRefExpr {{.*}} 'lu' 'union U':'union U'
  // DMP-PARLOOP-CCiCoFP-NEXT:    DeclRefExpr {{.*}} 'lUnref' 'int'
  // DMP-PARLOOP-C-NEXT:        ACCCopyClause
  // DMP-PARLOOP-Ci-NEXT:       ACCCopyinClause
  // DMP-PARLOOP-Co-NEXT:       ACCCopyoutClause
  // DMP-PARLOOP-F-NEXT:        ACCFirstprivateClause
  // DMP-PARLOOP-P-NEXT:        ACCPrivateClause
  // DMP-PARLOOP-CCiCoFP-NOT:     <implicit>
  // DMP-PARLOOP-CCiCoFP-SAME:    {{$}}
  // DMP-PARLOOP-CCiCoFP-NEXT:    DeclRefExpr {{.*}} 'shadowed' 'int'
  // DMP-PARLOOP-CCiCoF-NEXT:     DeclRefExpr {{.*}} 'ci' 'const int'
  // DMP-PARLOOP-CCiCoF-NEXT:     DeclRefExpr {{.*}} 'ca' 'const int [3]'
                                  // The test omits const when testing private.
  // DMP-PARLOOP-P-NEXT:          DeclRefExpr {{.*}} 'ci' 'int'
  // DMP-PARLOOP-P-NEXT:          DeclRefExpr {{.*}} 'ca' 'int [3]'
  // DMP-PARLOOP-NEXT:          effect: ACCParallelDirective
  // DMP-PAR:                   ACCParallelDirective
  // DMP-NEXT:                    ACCNum_gangsClause
  // DMP-NEXT:                      IntegerLiteral {{.*}} 'int' 2
  // DMP-C-NEXT:                  ACCCopyClause
  // DMP-Ci-NEXT:                 ACCCopyinClause
  // DMP-Co-NEXT:                 ACCCopyoutClause
  // DMP-F-NEXT:                  ACCFirstprivateClause
  // DMP-CCiCoF-NOT:                <implicit>
  // DMP-CCiCoF-SAME:               {{$}}
  // DMP-CCiCoF-NEXT:               DeclRefExpr {{.*}} 'gi' 'int'
  // DMP-CCiCoF-NEXT:               DeclRefExpr {{.*}} {{'gt' '__uint128_t'|'gt_hi' 'uint64_t'$[[:space:]]*DeclRefExpr .* 'gt_lw' 'uint64_t'}}
  // DMP-CCiCoF-NEXT:               DeclRefExpr {{.*}} 'gp' 'const int *'
  // DMP-CCiCoF-NEXT:               DeclRefExpr {{.*}} 'ga' 'int [2]'
  // DMP-CCiCoF-NEXT:               DeclRefExpr {{.*}} 'gs' 'struct S':'struct S'
  // DMP-CCiCoF-NEXT:               DeclRefExpr {{.*}} 'gu' 'union U':'union U'
  // DMP-CCiCoF-NEXT:               DeclRefExpr {{.*}} 'gUnref' 'int'
  // DMP-C-NEXT:                  ACCCopyClause
  // DMP-Ci-NEXT:                 ACCCopyinClause
  // DMP-Co-NEXT:                 ACCCopyoutClause
  // DMP-F-NEXT:                  ACCFirstprivateClause
  // DMP-CCiCoF-NOT:                <implicit>
  // DMP-CCiCoF-SAME:               {{$}}
  // DMP-CCiCoF-NEXT:               DeclRefExpr {{.*}} 'li' 'int'
  // DMP-CCiCoF-NEXT:               DeclRefExpr {{.*}} {{'lt' '__uint128_t'|'lt_hi' 'uint64_t'$[[:space:]]*DeclRefExpr .* 'lt_lw' 'uint64_t'}}
  // DMP-CCiCoF-NEXT:               DeclRefExpr {{.*}} 'lp' 'const int *'
  // DMP-CCiCoF-NEXT:               DeclRefExpr {{.*}} 'la' 'int [2]'
  // DMP-CCiCoF-NEXT:               DeclRefExpr {{.*}} 'ls' 'struct S':'struct S'
  // DMP-CCiCoF-NEXT:               DeclRefExpr {{.*}} 'lu' 'union U':'union U'
  // DMP-CCiCoF-NEXT:               DeclRefExpr {{.*}} 'lUnref' 'int'
  // DMP-C-NEXT:                  ACCCopyClause
  // DMP-Ci-NEXT:                 ACCCopyinClause
  // DMP-Co-NEXT:                 ACCCopyoutClause
  // DMP-F-NEXT:                  ACCFirstprivateClause
  // DMP-CCiCoF-NOT:                <implicit>
  // DMP-CCiCoF-SAME:               {{$}}
  // DMP-CCiCoF-NEXT:               DeclRefExpr {{.*}} 'shadowed' 'int'
  // DMP-CCiCoF-NEXT:               DeclRefExpr {{.*}} 'ci' 'const int'
  // DMP-CCiCoF-NEXT:               DeclRefExpr {{.*}} 'ca' 'const int [3]'
  // DMP-PAR-P-NEXT:              ACCPrivateClause
  // DMP-PAR-P-NOT:                 <implicit>
  // DMP-PAR-P-SAME:                {{$}}
  // DMP-PAR-P-NEXT:                DeclRefExpr {{.*}} 'gi' 'int'
  // DMP-PAR-P-NEXT:                DeclRefExpr {{.*}} {{'gt' '__uint128_t'|'gt_hi' 'uint64_t'$[[:space:]]*DeclRefExpr .* 'gt_lw' 'uint64_t'}}
  // DMP-PAR-P-NEXT:                DeclRefExpr {{.*}} 'gp' 'const int *'
  // DMP-PAR-P-NEXT:                DeclRefExpr {{.*}} 'ga' 'int [2]'
  // DMP-PAR-P-NEXT:                DeclRefExpr {{.*}} 'gs' 'struct S':'struct S'
  // DMP-PAR-P-NEXT:                DeclRefExpr {{.*}} 'gu' 'union U':'union U'
  // DMP-PAR-P-NEXT:                DeclRefExpr {{.*}} 'gUnref' 'int'
  // DMP-PAR-P-NEXT:              ACCPrivateClause
  // DMP-PAR-P-NOT:                 <implicit>
  // DMP-PAR-P-SAME:                {{$}}
  // DMP-PAR-P-NEXT:                DeclRefExpr {{.*}} 'li' 'int'
  // DMP-PAR-P-NEXT:                DeclRefExpr {{.*}} {{'lt' '__uint128_t'|'lt_hi' 'uint64_t'$[[:space:]]*DeclRefExpr .* 'lt_lw' 'uint64_t'}}
  // DMP-PAR-P-NEXT:                DeclRefExpr {{.*}} 'lp' 'const int *'
  // DMP-PAR-P-NEXT:                DeclRefExpr {{.*}} 'la' 'int [2]'
  // DMP-PAR-P-NEXT:                DeclRefExpr {{.*}} 'ls' 'struct S':'struct S'
  // DMP-PAR-P-NEXT:                DeclRefExpr {{.*}} 'lu' 'union U':'union U'
  // DMP-PAR-P-NEXT:                DeclRefExpr {{.*}} 'lUnref' 'int'
  // DMP-PAR-P-NEXT:              ACCPrivateClause
  // DMP-PAR-P-NOT:                 <implicit>
  // DMP-PAR-P-SAME:                {{$}}
  // DMP-PAR-P-NEXT:                DeclRefExpr {{.*}} 'shadowed' 'int'
                                    // The test omits const when testing private.
  // DMP-PAR-P-NEXT:                DeclRefExpr {{.*}} 'ci' 'int'
  // DMP-PAR-P-NEXT:                DeclRefExpr {{.*}} 'ca' 'int [3]'
  // DMP-I-NEXT:                  ACCCopyClause {{.*}} <implicit>
  // DMP-I-NEXT:                    DeclRefExpr {{.*}} 'ga' 'int [2]'
  // DMP-I-NEXT:                    DeclRefExpr {{.*}} 'gs' 'struct S':'struct S'
  // DMP-I-NEXT:                    DeclRefExpr {{.*}} 'gu' 'union U':'union U'
  // DMP-I-NEXT:                    DeclRefExpr {{.*}} 'la' 'int [2]'
  // DMP-I-NEXT:                    DeclRefExpr {{.*}} 'ls' 'struct S':'struct S'
  // DMP-I-NEXT:                    DeclRefExpr {{.*}} 'lu' 'union U':'union U'
  // DMP-I-NEXT:                    DeclRefExpr {{.*}} 'ca' 'const int [3]'
  // DMP-I-NEXT:                  ACCFirstprivateClause {{.*}} <implicit>
  // DMP-I-NEXT:                    DeclRefExpr {{.*}} 'gi' 'int'
  // DMP-I-NEXT:                    DeclRefExpr {{.*}} {{'gt' '__uint128_t'|'gt_hi' 'uint64_t'$[[:space:]]*DeclRefExpr .* 'gt_lw' 'uint64_t'}}
  // DMP-I-NEXT:                    DeclRefExpr {{.*}} 'gp' 'const int *'
  // DMP-I-NEXT:                    DeclRefExpr {{.*}} 'li' 'int'
  // DMP-I-NEXT:                    DeclRefExpr {{.*}} {{'lt' '__uint128_t'|'lt_hi' 'uint64_t'$[[:space:]]*DeclRefExpr .* 'lt_lw' 'uint64_t'}}
  // DMP-I-NEXT:                    DeclRefExpr {{.*}} 'lp' 'const int *'
  // DMP-I-NEXT:                    DeclRefExpr {{.*}} 'shadowed' 'int'
  // DMP-I-NEXT:                    DeclRefExpr {{.*}} 'ci' 'const int'
  // DMP-NEXT:                    impl: OMPTargetTeamsDirective
  // DMP-NEXT:                      OMPNum_teamsClause
  // DMP-NEXT:                        IntegerLiteral {{.*}} 'int' 2
  // DMP-CCiCo-NEXT:                OMPMapClause
  // DMP-F-NEXT:                    OMPFirstprivateClause
  // DMP-CCiCoF-NOT:                  <implicit>
  // DMP-CCiCoF-SAME:                 {{$}}
  // DMP-CCiCoF-NEXT:                 DeclRefExpr {{.*}} 'gi' 'int'
  // DMP-CCiCoF-NEXT:                 DeclRefExpr {{.*}} {{'gt' '__uint128_t'|'gt_hi' 'uint64_t'$[[:space:]]*DeclRefExpr .* 'gt_lw' 'uint64_t'}}
  // DMP-CCiCoF-NEXT:                 DeclRefExpr {{.*}} 'gp' 'const int *'
  // DMP-CCiCoF-NEXT:                 DeclRefExpr {{.*}} 'ga' 'int [2]'
  // DMP-CCiCoF-NEXT:                 DeclRefExpr {{.*}} 'gs' 'struct S':'struct S'
  // DMP-CCiCoF-NEXT:                 DeclRefExpr {{.*}} 'gu' 'union U':'union U'
  // DMP-CCiCoF-NEXT:                 DeclRefExpr {{.*}} 'gUnref' 'int'
  // DMP-CCiCo-NEXT:                OMPMapClause
  // DMP-F-NEXT:                    OMPFirstprivateClause
  // DMP-CCiCoF-NOT:                  <implicit>
  // DMP-CCiCoF-SAME:                 {{$}}
  // DMP-CCiCoF-NEXT:                 DeclRefExpr {{.*}} 'li' 'int'
  // DMP-CCiCoF-NEXT:                 DeclRefExpr {{.*}} {{'lt' '__uint128_t'|'lt_hi' 'uint64_t'$[[:space:]]*DeclRefExpr .* 'lt_lw' 'uint64_t'}}
  // DMP-CCiCoF-NEXT:                 DeclRefExpr {{.*}} 'lp' 'const int *'
  // DMP-CCiCoF-NEXT:                 DeclRefExpr {{.*}} 'la' 'int [2]'
  // DMP-CCiCoF-NEXT:                 DeclRefExpr {{.*}} 'ls' 'struct S':'struct S'
  // DMP-CCiCoF-NEXT:                 DeclRefExpr {{.*}} 'lu' 'union U':'union U'
  // DMP-CCiCoF-NEXT:                 DeclRefExpr {{.*}} 'lUnref' 'int'
  // DMP-CCiCo-NEXT:                OMPMapClause
  // DMP-F-NEXT:                    OMPFirstprivateClause
  // DMP-CCiCoF-NOT:                  <implicit>
  // DMP-CCiCoF-SAME:                 {{$}}
  // DMP-CCiCoF-NEXT:                 DeclRefExpr {{.*}} 'shadowed' 'int'
  // DMP-CCiCoF-NEXT:                 DeclRefExpr {{.*}} 'ci' 'const int'
  // DMP-CCiCoF-NEXT:                 DeclRefExpr {{.*}} 'ca' 'const int [3]'
  // DMP-PAR-P-NEXT:                OMPPrivateClause
  // DMP-PAR-P-NOT:                   <implicit>
  // DMP-PAR-P-SAME:                  {{$}}
  // DMP-PAR-P-NEXT:                  DeclRefExpr {{.*}} 'gi' 'int'
  // DMP-PAR-P-NEXT:                  DeclRefExpr {{.*}} {{'gt' '__uint128_t'|'gt_hi' 'uint64_t'$[[:space:]]*DeclRefExpr .* 'gt_lw' 'uint64_t'}}
  // DMP-PAR-P-NEXT:                  DeclRefExpr {{.*}} 'gp' 'const int *'
  // DMP-PAR-P-NEXT:                  DeclRefExpr {{.*}} 'ga' 'int [2]'
  // DMP-PAR-P-NEXT:                  DeclRefExpr {{.*}} 'gs' 'struct S':'struct S'
  // DMP-PAR-P-NEXT:                  DeclRefExpr {{.*}} 'gu' 'union U':'union U'
  // DMP-PAR-P-NEXT:                  DeclRefExpr {{.*}} 'gUnref' 'int'
  // DMP-PAR-P-NEXT:                OMPPrivateClause
  // DMP-PAR-P-NOT:                   <implicit>
  // DMP-PAR-P-SAME:                  {{$}}
  // DMP-PAR-P-NEXT:                  DeclRefExpr {{.*}} 'li' 'int'
  // DMP-PAR-P-NEXT:                  DeclRefExpr {{.*}} {{'lt' '__uint128_t'|'lt_hi' 'uint64_t'$[[:space:]]*DeclRefExpr .* 'lt_lw' 'uint64_t'}}
  // DMP-PAR-P-NEXT:                  DeclRefExpr {{.*}} 'lp' 'const int *'
  // DMP-PAR-P-NEXT:                  DeclRefExpr {{.*}} 'la' 'int [2]'
  // DMP-PAR-P-NEXT:                  DeclRefExpr {{.*}} 'ls' 'struct S':'struct S'
  // DMP-PAR-P-NEXT:                  DeclRefExpr {{.*}} 'lu' 'union U':'union U'
  // DMP-PAR-P-NEXT:                  DeclRefExpr {{.*}} 'lUnref' 'int'
  // DMP-PAR-P-NEXT:                OMPPrivateClause
  // DMP-PAR-P-NOT:                   <implicit>
  // DMP-PAR-P-SAME:                  {{$}}
  // DMP-PAR-P-NEXT:                  DeclRefExpr {{.*}} 'shadowed' 'int'
                                      // The test omits const when testing private.
  // DMP-PAR-P-NEXT:                  DeclRefExpr {{.*}} 'ci' 'int'
  // DMP-PAR-P-NEXT:                  DeclRefExpr {{.*}} 'ca' 'int [3]'
  // DMP-I-NEXT:                    OMPMapClause
  // DMP-I-NOT:                       <implicit>
  // DMP-I-SAME:                      {{$}}
  // DMP-I-NEXT:                      DeclRefExpr {{.*}} 'ga' 'int [2]'
  // DMP-I-NEXT:                      DeclRefExpr {{.*}} 'gs' 'struct S':'struct S'
  // DMP-I-NEXT:                      DeclRefExpr {{.*}} 'gu' 'union U':'union U'
  // DMP-I-NEXT:                      DeclRefExpr {{.*}} 'la' 'int [2]'
  // DMP-I-NEXT:                      DeclRefExpr {{.*}} 'ls' 'struct S':'struct S'
  // DMP-I-NEXT:                      DeclRefExpr {{.*}} 'lu' 'union U':'union U'
  // DMP-I-NEXT:                      DeclRefExpr {{.*}} 'ca' 'const int [3]'
  // DMP-I-NEXT:                    OMPFirstprivateClause
  // DMP-I-NOT:                       <implicit>
  // DMP-I-SAME:                      {{$}}
  // DMP-I-NEXT:                      DeclRefExpr {{.*}} 'gi' 'int'
  // DMP-I-NEXT:                      DeclRefExpr {{.*}} {{'gt' '__uint128_t'|'gt_hi' 'uint64_t'$[[:space:]]*DeclRefExpr .* 'gt_lw' 'uint64_t'}}
  // DMP-I-NEXT:                      DeclRefExpr {{.*}} 'gp' 'const int *'
  // DMP-I-NEXT:                      DeclRefExpr {{.*}} 'li' 'int'
  // DMP-I-NEXT:                      DeclRefExpr {{.*}} {{'lt' '__uint128_t'|'lt_hi' 'uint64_t'$[[:space:]]*DeclRefExpr .* 'lt_lw' 'uint64_t'}}
  // DMP-I-NEXT:                      DeclRefExpr {{.*}} 'lp' 'const int *'
  // DMP-I-NEXT:                      DeclRefExpr {{.*}} 'shadowed' 'int'
  // DMP-I-NEXT:                      DeclRefExpr {{.*}} 'ci' 'const int'
  // DMP-PARLOOP:                 ACCLoopDirective
  // DMP-PARLOOP-NEXT:              ACCSeqClause
  // DMP-PARLOOP-P-NEXT:            ACCPrivateClause
  // DMP-PARLOOP-P-NOT:               <implicit>
  // DMP-PARLOOP-P-SAME:              {{$}}
  // DMP-PARLOOP-P-NEXT:              DeclRefExpr {{.*}} 'gi' 'int'
  // DMP-PARLOOP-P-NEXT:              DeclRefExpr {{.*}} {{'gt' '__uint128_t'|'gt_hi' 'uint64_t'$[[:space:]]*DeclRefExpr .* 'gt_lw' 'uint64_t'}}
  // DMP-PARLOOP-P-NEXT:              DeclRefExpr {{.*}} 'gp' 'const int *'
  // DMP-PARLOOP-P-NEXT:              DeclRefExpr {{.*}} 'ga' 'int [2]'
  // DMP-PARLOOP-P-NEXT:              DeclRefExpr {{.*}} 'gs' 'struct S':'struct S'
  // DMP-PARLOOP-P-NEXT:              DeclRefExpr {{.*}} 'gu' 'union U':'union U'
  // DMP-PARLOOP-P-NEXT:              DeclRefExpr {{.*}} 'gUnref' 'int'
  // DMP-PARLOOP-P-NEXT:            ACCPrivateClause
  // DMP-PARLOOP-P-NOT:               <implicit>
  // DMP-PARLOOP-P-SAME:              {{$}}
  // DMP-PARLOOP-P-NEXT:              DeclRefExpr {{.*}} 'li' 'int'
  // DMP-PARLOOP-P-NEXT:              DeclRefExpr {{.*}} {{'lt' '__uint128_t'|'lt_hi' 'uint64_t'$[[:space:]]*DeclRefExpr .* 'lt_lw' 'uint64_t'}}
  // DMP-PARLOOP-P-NEXT:              DeclRefExpr {{.*}} 'lp' 'const int *'
  // DMP-PARLOOP-P-NEXT:              DeclRefExpr {{.*}} 'la' 'int [2]'
  // DMP-PARLOOP-P-NEXT:              DeclRefExpr {{.*}} 'ls' 'struct S':'struct S'
  // DMP-PARLOOP-P-NEXT:              DeclRefExpr {{.*}} 'lu' 'union U':'union U'
  // DMP-PARLOOP-P-NEXT:              DeclRefExpr {{.*}} 'lUnref' 'int'
  // DMP-PARLOOP-P-NEXT:            ACCPrivateClause
  // DMP-PARLOOP-P-NOT:               <implicit>
  // DMP-PARLOOP-P-SAME:              {{$}}
  // DMP-PARLOOP-P-NEXT:              DeclRefExpr {{.*}} 'shadowed' 'int'
                                      // The test omits const when testing private.
  // DMP-PARLOOP-P-NEXT:              DeclRefExpr {{.*}} 'ci' 'int'
  // DMP-PARLOOP-P-NEXT:              DeclRefExpr {{.*}} 'ca' 'int [3]'
  // DMP-PARLOOP-ICCiCoF-NEXT:      ACCSharedClause {{.*}} <implicit>
  // DMP-PARLOOP-ICCiCoF-NEXT:        DeclRefExpr {{.*}} 'gi' 'int'
  // DMP-PARLOOP-ICCiCoF-NEXT:        DeclRefExpr {{.*}} {{'gt' '__uint128_t'|'gt_hi' 'uint64_t'$[[:space:]]*DeclRefExpr .* 'gt_lw' 'uint64_t'}}
  // DMP-PARLOOP-ICCiCoF-NEXT:        DeclRefExpr {{.*}} 'gp' 'const int *'
  // DMP-PARLOOP-ICCiCoF-NEXT:        DeclRefExpr {{.*}} 'ga' 'int [2]'
  // DMP-PARLOOP-ICCiCoF-NEXT:        DeclRefExpr {{.*}} 'gs' 'struct S':'struct S'
  // DMP-PARLOOP-ICCiCoF-NEXT:        DeclRefExpr {{.*}} 'gu' 'union U':'union U'
  // DMP-PARLOOP-ICCiCoF-NEXT:        DeclRefExpr {{.*}} 'li' 'int'
  // DMP-PARLOOP-ICCiCoF-NEXT:        DeclRefExpr {{.*}} {{'lt' '__uint128_t'|'lt_hi' 'uint64_t'$[[:space:]]*DeclRefExpr .* 'lt_lw' 'uint64_t'}}
  // DMP-PARLOOP-ICCiCoF-NEXT:        DeclRefExpr {{.*}} 'lp' 'const int *'
  // DMP-PARLOOP-ICCiCoF-NEXT:        DeclRefExpr {{.*}} 'la' 'int [2]'
  // DMP-PARLOOP-ICCiCoF-NEXT:        DeclRefExpr {{.*}} 'ls' 'struct S':'struct S'
  // DMP-PARLOOP-ICCiCoF-NEXT:        DeclRefExpr {{.*}} 'lu' 'union U':'union U'
  // DMP-PARLOOP-ICCiCoF-NEXT:        DeclRefExpr {{.*}} 'shadowed' 'int'
  // DMP-PARLOOP-ICCiCoF-NEXT:        DeclRefExpr {{.*}} 'ci' 'const int'
  // DMP-PARLOOP-ICCiCoF-NEXT:        DeclRefExpr {{.*}} 'ca' 'const int [3]'
  // DMP-PARLOOP-ICCiCoF-NEXT:      impl: ForStmt
  // DMP-PARLOOP-P-NEXT:            impl: CompoundStmt
  // DMP-PARLOOP-P-NEXT:              DeclStmt
  // DMP-PARLOOP-P-NEXT:                VarDecl {{.*}} gi 'int'
  // DMP-PARLOOP-P-NEXT:              DeclStmt
  // DMP-PARLOOP-P-NEXT:                VarDecl {{.*}} {{gt '__uint128_t'|gt_hi 'uint64_t'$[[:space:]]*VarDecl .* gt_lw 'uint64_t'}}
  // DMP-PARLOOP-P-NEXT:              DeclStmt
  // DMP-PARLOOP-P-NEXT:                VarDecl {{.*}} gp 'const int *'
  // DMP-PARLOOP-P-NEXT:              DeclStmt
  // DMP-PARLOOP-P-NEXT:                VarDecl {{.*}} ga 'int [2]'
  // DMP-PARLOOP-P-NEXT:              DeclStmt
  // DMP-PARLOOP-P-NEXT:                VarDecl {{.*}} gs 'struct S':'struct S'
  // DMP-PARLOOP-P-NEXT:              DeclStmt
  // DMP-PARLOOP-P-NEXT:                VarDecl {{.*}} gu 'union U':'union U'
  // DMP-PARLOOP-P-NEXT:              DeclStmt
  // DMP-PARLOOP-P-NEXT:                VarDecl {{.*}} gUnref 'int'
  // DMP-PARLOOP-P-NEXT:              DeclStmt
  // DMP-PARLOOP-P-NEXT:                VarDecl {{.*}} li 'int'
  // DMP-PARLOOP-P-NEXT:              DeclStmt
  // DMP-PARLOOP-P-NEXT:                VarDecl {{.*}} {{lt '__uint128_t'|lt_hi 'uint64_t'$[[:space:]]*VarDecl .* lt_lw 'uint64_t'}}
  // DMP-PARLOOP-P-NEXT:              DeclStmt
  // DMP-PARLOOP-P-NEXT:                VarDecl {{.*}} lp 'const int *'
  // DMP-PARLOOP-P-NEXT:              DeclStmt
  // DMP-PARLOOP-P-NEXT:                VarDecl {{.*}} la 'int [2]'
  // DMP-PARLOOP-P-NEXT:              DeclStmt
  // DMP-PARLOOP-P-NEXT:                VarDecl {{.*}} ls 'struct S':'struct S'
  // DMP-PARLOOP-P-NEXT:              DeclStmt
  // DMP-PARLOOP-P-NEXT:                VarDecl {{.*}} lu 'union U':'union U'
  // DMP-PARLOOP-P-NEXT:              DeclStmt
  // DMP-PARLOOP-P-NEXT:                VarDecl {{.*}} lUnref 'int'
  // DMP-PARLOOP-P-NEXT:              DeclStmt
  // DMP-PARLOOP-P-NEXT:                VarDecl {{.*}} shadowed 'int'
                                      // The test omits const when testing private.
  // DMP-PARLOOP-P-NEXT:              DeclStmt
  // DMP-PARLOOP-P-NEXT:                VarDecl {{.*}} ci 'int'
  // DMP-PARLOOP-P-NEXT:              DeclStmt
  // DMP-PARLOOP-P-NEXT:                VarDecl {{.*}} ca 'int [3]'
  // DMP-PARLOOP-P-NEXT:              ForStmt
  //
  // PRT-NOT:       #pragma
  //
  // PRT-A-I:                {{^ *}}#pragma acc parallel{{ LOOP | loop seq | }}num_gangs(2){{(.*\\$[[:space:]])*.*$}}
  // PRT-AO-I-NEXT:          {{^ *}}// #pragma omp target teams num_teams(2) map(tofrom: ga,gs,gu,la,ls,lu,ca) firstprivate(gi,gt{{(_hi,gt_lw)?}},gp,li,lt{{(_hi,lt_lw)?}},lp,shadowed,ci){{$}}
  // PRT-O-I:                {{^ *}}#pragma omp target teams num_teams(2) map(tofrom: ga,gs,gu,la,ls,lu,ca) firstprivate(gi,gt{{(_hi,gt_lw)?}},gp,li,lt{{(_hi,lt_lw)?}},lp,shadowed,ci){{$}}
  // PRT-OA-I-NEXT:          {{^ *}}// #pragma acc parallel{{ LOOP | loop seq | }}num_gangs(2){{(.*\\$[[:space:]])*.*$}}
  //
  // PRT-A-C0:               {{^ *}}#pragma acc parallel{{ LOOP | loop seq | }}num_gangs(2) {{copy\(gi,gt(_hi,gt_lw)?,gp,ga,gs,gu,gUnref\) copy\(li,lt(_hi,lt_lw)?,lp,la,ls,lu,lUnref\) copy\(shadowed,ci,ca\)$|(.*\\$[[:space:]])+.*$}}
  // PRT-A-C1:               {{^ *}}#pragma acc parallel{{ LOOP | loop seq | }}num_gangs(2) {{pcopy\(gi,gt(_hi,gt_lw)?,gp,ga,gs,gu,gUnref\) pcopy\(li,lt(_hi,lt_lw)?,lp,la,ls,lu,lUnref\) pcopy\(shadowed,ci,ca\)$|(.*\\$[[:space:]])+.*$}}
  // PRT-A-C2:               {{^ *}}#pragma acc parallel{{ LOOP | loop seq | }}num_gangs(2) {{present_or_copy\(gi,gt(_hi,gt_lw)?,gp,ga,gs,gu,gUnref\) present_or_copy\(li,lt(_hi,lt_lw)?,lp,la,ls,lu,lUnref\) present_or_copy\(shadowed,ci,ca\)$|(.*\\$[[:space:]])+.*$}}
  // PRT-A-Ci0:              {{^ *}}#pragma acc parallel{{ LOOP | loop seq | }}num_gangs(2) {{copyin\(gi,gt(_hi,gt_lw)?,gp,ga,gs,gu,gUnref\) copyin\(li,lt(_hi,lt_lw)?,lp,la,ls,lu,lUnref\) copyin\(shadowed,ci,ca\)$|(.*\\$[[:space:]])+.*$}}
  // PRT-A-Ci1:              {{^ *}}#pragma acc parallel{{ LOOP | loop seq | }}num_gangs(2) {{pcopyin\(gi,gt(_hi,gt_lw)?,gp,ga,gs,gu,gUnref\) pcopyin\(li,lt(_hi,lt_lw)?,lp,la,ls,lu,lUnref\) pcopyin\(shadowed,ci,ca\)$|(.*\\$[[:space:]])+.*$}}
  // PRT-A-Ci2:              {{^ *}}#pragma acc parallel{{ LOOP | loop seq | }}num_gangs(2) {{present_or_copyin\(gi,gt(_hi,gt_lw)?,gp,ga,gs,gu,gUnref\) present_or_copyin\(li,lt(_hi,lt_lw)?,lp,la,ls,lu,lUnref\) present_or_copyin\(shadowed,ci,ca\)$|(.*\\$[[:space:]])+.*$}}
  // PRT-A-Co0:              {{^ *}}#pragma acc parallel{{ LOOP | loop seq | }}num_gangs(2) {{copyout\(gi,gt(_hi,gt_lw)?,gp,ga,gs,gu,gUnref\) copyout\(li,lt(_hi,lt_lw)?,lp,la,ls,lu,lUnref\) copyout\(shadowed,ci,ca\)$|(.*\\$[[:space:]])+.*$}}
  // PRT-A-Co1:              {{^ *}}#pragma acc parallel{{ LOOP | loop seq | }}num_gangs(2) {{pcopyout\(gi,gt(_hi,gt_lw)?,gp,ga,gs,gu,gUnref\) pcopyout\(li,lt(_hi,lt_lw)?,lp,la,ls,lu,lUnref\) pcopyout\(shadowed,ci,ca\)$|(.*\\$[[:space:]])+.*$}}
  // PRT-A-Co2:              {{^ *}}#pragma acc parallel{{ LOOP | loop seq | }}num_gangs(2) {{present_or_copyout\(gi,gt(_hi,gt_lw)?,gp,ga,gs,gu,gUnref\) present_or_copyout\(li,lt(_hi,lt_lw)?,lp,la,ls,lu,lUnref\) present_or_copyout\(shadowed,ci,ca\)$|(.*\\$[[:space:]])+.*$}}
  // PRT-AO-C0-NEXT:         {{^ *}}// #pragma omp target teams num_teams(2) map(tofrom: gi,gt{{(_hi,gt_lw)?}},gp,ga,gs,gu,gUnref) map(tofrom: li,lt{{(_hi,lt_lw)?}},lp,la,ls,lu,lUnref) map(tofrom: shadowed,ci,ca){{$}}
  // PRT-AO-C1-NEXT:         {{^ *}}// #pragma omp target teams num_teams(2) map(tofrom: gi,gt{{(_hi,gt_lw)?}},gp,ga,gs,gu,gUnref) map(tofrom: li,lt{{(_hi,lt_lw)?}},lp,la,ls,lu,lUnref) map(tofrom: shadowed,ci,ca){{$}}
  // PRT-AO-C2-NEXT:         {{^ *}}// #pragma omp target teams num_teams(2) map(tofrom: gi,gt{{(_hi,gt_lw)?}},gp,ga,gs,gu,gUnref) map(tofrom: li,lt{{(_hi,lt_lw)?}},lp,la,ls,lu,lUnref) map(tofrom: shadowed,ci,ca){{$}}
  // PRT-AO-Ci0-NEXT:        {{^ *}}// #pragma omp target teams num_teams(2) map(to: gi,gt{{(_hi,gt_lw)?}},gp,ga,gs,gu,gUnref) map(to: li,lt{{(_hi,lt_lw)?}},lp,la,ls,lu,lUnref) map(to: shadowed,ci,ca){{$}}
  // PRT-AO-Ci1-NEXT:        {{^ *}}// #pragma omp target teams num_teams(2) map(to: gi,gt{{(_hi,gt_lw)?}},gp,ga,gs,gu,gUnref) map(to: li,lt{{(_hi,lt_lw)?}},lp,la,ls,lu,lUnref) map(to: shadowed,ci,ca){{$}}
  // PRT-AO-Ci2-NEXT:        {{^ *}}// #pragma omp target teams num_teams(2) map(to: gi,gt{{(_hi,gt_lw)?}},gp,ga,gs,gu,gUnref) map(to: li,lt{{(_hi,lt_lw)?}},lp,la,ls,lu,lUnref) map(to: shadowed,ci,ca){{$}}
  // PRT-AO-Co0-NEXT:        {{^ *}}// #pragma omp target teams num_teams(2) map(from: gi,gt{{(_hi,gt_lw)?}},gp,ga,gs,gu,gUnref) map(from: li,lt{{(_hi,lt_lw)?}},lp,la,ls,lu,lUnref) map(from: shadowed,ci,ca){{$}}
  // PRT-AO-Co1-NEXT:        {{^ *}}// #pragma omp target teams num_teams(2) map(from: gi,gt{{(_hi,gt_lw)?}},gp,ga,gs,gu,gUnref) map(from: li,lt{{(_hi,lt_lw)?}},lp,la,ls,lu,lUnref) map(from: shadowed,ci,ca){{$}}
  // PRT-AO-Co2-NEXT:        {{^ *}}// #pragma omp target teams num_teams(2) map(from: gi,gt{{(_hi,gt_lw)?}},gp,ga,gs,gu,gUnref) map(from: li,lt{{(_hi,lt_lw)?}},lp,la,ls,lu,lUnref) map(from: shadowed,ci,ca){{$}}
  // PRT-O-C0:               {{^ *}}#pragma omp target teams num_teams(2) map(tofrom: gi,gt{{(_hi,gt_lw)?}},gp,ga,gs,gu,gUnref) map(tofrom: li,lt{{(_hi,lt_lw)?}},lp,la,ls,lu,lUnref) map(tofrom: shadowed,ci,ca){{$}}
  // PRT-O-C1:               {{^ *}}#pragma omp target teams num_teams(2) map(tofrom: gi,gt{{(_hi,gt_lw)?}},gp,ga,gs,gu,gUnref) map(tofrom: li,lt{{(_hi,lt_lw)?}},lp,la,ls,lu,lUnref) map(tofrom: shadowed,ci,ca){{$}}
  // PRT-O-C2:               {{^ *}}#pragma omp target teams num_teams(2) map(tofrom: gi,gt{{(_hi,gt_lw)?}},gp,ga,gs,gu,gUnref) map(tofrom: li,lt{{(_hi,lt_lw)?}},lp,la,ls,lu,lUnref) map(tofrom: shadowed,ci,ca){{$}}
  // PRT-O-Ci0:              {{^ *}}#pragma omp target teams num_teams(2) map(to: gi,gt{{(_hi,gt_lw)?}},gp,ga,gs,gu,gUnref) map(to: li,lt{{(_hi,lt_lw)?}},lp,la,ls,lu,lUnref) map(to: shadowed,ci,ca){{$}}
  // PRT-O-Ci1:              {{^ *}}#pragma omp target teams num_teams(2) map(to: gi,gt{{(_hi,gt_lw)?}},gp,ga,gs,gu,gUnref) map(to: li,lt{{(_hi,lt_lw)?}},lp,la,ls,lu,lUnref) map(to: shadowed,ci,ca){{$}}
  // PRT-O-Ci2:              {{^ *}}#pragma omp target teams num_teams(2) map(to: gi,gt{{(_hi,gt_lw)?}},gp,ga,gs,gu,gUnref) map(to: li,lt{{(_hi,lt_lw)?}},lp,la,ls,lu,lUnref) map(to: shadowed,ci,ca){{$}}
  // PRT-O-Co0:              {{^ *}}#pragma omp target teams num_teams(2) map(from: gi,gt{{(_hi,gt_lw)?}},gp,ga,gs,gu,gUnref) map(from: li,lt{{(_hi,lt_lw)?}},lp,la,ls,lu,lUnref) map(from: shadowed,ci,ca){{$}}
  // PRT-O-Co1:              {{^ *}}#pragma omp target teams num_teams(2) map(from: gi,gt{{(_hi,gt_lw)?}},gp,ga,gs,gu,gUnref) map(from: li,lt{{(_hi,lt_lw)?}},lp,la,ls,lu,lUnref) map(from: shadowed,ci,ca){{$}}
  // PRT-O-Co2:              {{^ *}}#pragma omp target teams num_teams(2) map(from: gi,gt{{(_hi,gt_lw)?}},gp,ga,gs,gu,gUnref) map(from: li,lt{{(_hi,lt_lw)?}},lp,la,ls,lu,lUnref) map(from: shadowed,ci,ca){{$}}
  // PRT-OA-C0-NEXT:         {{^ *}}// #pragma acc parallel{{ LOOP | loop seq | }}num_gangs(2) {{copy\(gi,gt(_hi,gt_lw)?,gp,ga,gs,gu,gUnref\) copy\(li,lt(_hi,lt_lw)?,lp,la,ls,lu,lUnref\) copy\(shadowed,ci,ca\)$|(.*\\$[[:space:]])+.*$}}
  // PRT-OA-C1-NEXT:         {{^ *}}// #pragma acc parallel{{ LOOP | loop seq | }}num_gangs(2) {{pcopy\(gi,gt(_hi,gt_lw)?,gp,ga,gs,gu,gUnref\) pcopy\(li,lt(_hi,lt_lw)?,lp,la,ls,lu,lUnref\) pcopy\(shadowed,ci,ca\)$|(.*\\$[[:space:]])+.*$}}
  // PRT-OA-C2-NEXT:         {{^ *}}// #pragma acc parallel{{ LOOP | loop seq | }}num_gangs(2) {{present_or_copy\(gi,gt(_hi,gt_lw)?,gp,ga,gs,gu,gUnref\) present_or_copy\(li,lt(_hi,lt_lw)?,lp,la,ls,lu,lUnref\) present_or_copy\(shadowed,ci,ca\)$|(.*\\$[[:space:]])+.*$}}
  // PRT-OA-Ci0-NEXT:        {{^ *}}// #pragma acc parallel{{ LOOP | loop seq | }}num_gangs(2) {{copyin\(gi,gt(_hi,gt_lw)?,gp,ga,gs,gu,gUnref\) copyin\(li,lt(_hi,lt_lw)?,lp,la,ls,lu,lUnref\) copyin\(shadowed,ci,ca\)$|(.*\\$[[:space:]])+.*$}}
  // PRT-OA-Ci1-NEXT:        {{^ *}}// #pragma acc parallel{{ LOOP | loop seq | }}num_gangs(2) {{pcopyin\(gi,gt(_hi,gt_lw)?,gp,ga,gs,gu,gUnref\) pcopyin\(li,lt(_hi,lt_lw)?,lp,la,ls,lu,lUnref\) pcopyin\(shadowed,ci,ca\)$|(.*\\$[[:space:]])+.*$}}
  // PRT-OA-Ci2-NEXT:        {{^ *}}// #pragma acc parallel{{ LOOP | loop seq | }}num_gangs(2) {{present_or_copyin\(gi,gt(_hi,gt_lw)?,gp,ga,gs,gu,gUnref\) present_or_copyin\(li,lt(_hi,lt_lw)?,lp,la,ls,lu,lUnref\) present_or_copyin\(shadowed,ci,ca\)$|(.*\\$[[:space:]])+.*$}}
  // PRT-OA-Co0-NEXT:        {{^ *}}// #pragma acc parallel{{ LOOP | loop seq | }}num_gangs(2) {{copyout\(gi,gt(_hi,gt_lw)?,gp,ga,gs,gu,gUnref\) copyout\(li,lt(_hi,lt_lw)?,lp,la,ls,lu,lUnref\) copyout\(shadowed,ci,ca\)$|(.*\\$[[:space:]])+.*$}}
  // PRT-OA-Co1-NEXT:        {{^ *}}// #pragma acc parallel{{ LOOP | loop seq | }}num_gangs(2) {{pcopyout\(gi,gt(_hi,gt_lw)?,gp,ga,gs,gu,gUnref\) pcopyout\(li,lt(_hi,lt_lw)?,lp,la,ls,lu,lUnref\) pcopyout\(shadowed,ci,ca\)$|(.*\\$[[:space:]])+.*$}}
  // PRT-OA-Co2-NEXT:        {{^ *}}// #pragma acc parallel{{ LOOP | loop seq | }}num_gangs(2) {{present_or_copyout\(gi,gt(_hi,gt_lw)?,gp,ga,gs,gu,gUnref\) present_or_copyout\(li,lt(_hi,lt_lw)?,lp,la,ls,lu,lUnref\) present_or_copyout\(shadowed,ci,ca\)$|(.*\\$[[:space:]])+.*$}}
  //
  // PRT-A-F:                {{^ *}}#pragma acc parallel{{ LOOP | loop seq | }}num_gangs(2) {{firstprivate\(gi,gt(_hi,gt_lw)?,gp,ga,gs,gu,gUnref\) firstprivate\(li,lt(_hi,lt_lw)?,lp,la,ls,lu,lUnref\) firstprivate\(shadowed,ci,ca\)$|(.*\\$[[:space:]])+.*$}}
  // PRT-AO-F-NEXT:          {{^ *}}// #pragma omp target teams num_teams(2) firstprivate(gi,gt{{(_hi,gt_lw)?}},gp,ga,gs,gu,gUnref) firstprivate(li,lt{{(_hi,lt_lw)?}},lp,la,ls,lu,lUnref) firstprivate(shadowed,ci,ca){{$}}
  // PRT-O-F:                {{^ *}}#pragma omp target teams num_teams(2) firstprivate(gi,gt{{(_hi,gt_lw)?}},gp,ga,gs,gu,gUnref) firstprivate(li,lt{{(_hi,lt_lw)?}},lp,la,ls,lu,lUnref) firstprivate(shadowed,ci,ca){{$}}
  // PRT-OA-F-NEXT:          {{^ *}}// #pragma acc parallel{{ LOOP | loop seq | }}num_gangs(2) {{firstprivate\(gi,gt(_hi,gt_lw)?,gp,ga,gs,gu,gUnref\) firstprivate\(li,lt(_hi,lt_lw)?,lp,la,ls,lu,lUnref\) firstprivate\(shadowed,ci,ca\)$|(.*\\$[[:space:]])+.*$}}
  //
  // PRT-A-P:                {{^ *}}#pragma acc parallel{{ LOOP | loop seq | }}num_gangs(2) {{private\(gi,gt(_hi,gt_lw)?,gp,ga,gs,gu,gUnref\) private\(li,lt(_hi,lt_lw)?,lp,la,ls,lu,lUnref\) private\(shadowed,ci,ca\)$|(.*\\$[[:space:]])+.*$}}
  // PRT-AO-PAR-P-NEXT:      {{^ *}}// #pragma omp target teams num_teams(2) private(gi,gt{{(_hi,gt_lw)?}},gp,ga,gs,gu,gUnref) private(li,lt{{(_hi,lt_lw)?}},lp,la,ls,lu,lUnref) private(shadowed,ci,ca){{$}}
  // PRT-AO-PARLOOP-P-NOT:   #pragma
  // PRT-AO-PARLOOP-P:       {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  // PRT-AO-PARLOOP-P-NEXT:  //{{ *}}{
  // PRT-AO-PARLOOP-P-NEXT:  //{{ *}}  int gi;
  // PRT-AO-PARLOOP-P-NEXT:  //{{ *}}  {{__uint128_t gt;|uint64_t gt_hi;$[[:space:]]*// *  uint64_t gt_lw;}}
  // PRT-AO-PARLOOP-P-NEXT:  //{{ *}}  const int *gp;
  // PRT-AO-PARLOOP-P-NEXT:  //{{ *}}  int ga[2];
  // PRT-AO-PARLOOP-P-NEXT:  //{{ *}}  struct S gs;
  // PRT-AO-PARLOOP-P-NEXT:  //{{ *}}  union U gu;
  // PRT-AO-PARLOOP-P-NEXT:  //{{ *}}  int gUnref;
  // PRT-AO-PARLOOP-P-NEXT:  //{{ *}}  int li;
  // PRT-AO-PARLOOP-P-NEXT:  //{{ *}}  {{__uint128_t lt;|uint64_t lt_hi;$[[:space:]]*// *  uint64_t lt_lw;}}
  // PRT-AO-PARLOOP-P-NEXT:  //{{ *}}  const int *lp;
  // PRT-AO-PARLOOP-P-NEXT:  //{{ *}}  int la[2];
  // PRT-AO-PARLOOP-P-NEXT:  //{{ *}}  struct S ls;
  // PRT-AO-PARLOOP-P-NEXT:  //{{ *}}  union U lu;
  // PRT-AO-PARLOOP-P-NEXT:  //{{ *}}  int lUnref;
  // PRT-AO-PARLOOP-P-NEXT:  //{{ *}}  int shadowed;
                                       // The test omits const when testing
                                       // private.
  // PRT-AO-PARLOOP-P-NEXT:  //{{ *}}  int ci;
  // PRT-AO-PARLOOP-P-NEXT:  //{{ *}}  int ca[3];
  // PRT-O-PAR-P:            {{^ *}}#pragma omp target teams num_teams(2) private(gi,gt{{(_hi,gt_lw)?}},gp,ga,gs,gu,gUnref) private(li,lt{{(_hi,lt_lw)?}},lp,la,ls,lu,lUnref) private(shadowed,ci,ca){{$}}
  // PRT-OA-PAR-P-NEXT:      {{^ *}}// #pragma acc parallel{{ LOOP | }}num_gangs(2) {{private\(gi,gt(_hi,gt_lw)?,gp,ga,gs,gu,gUnref\) private\(li,lt(_hi,lt_lw)?,lp,la,ls,lu,lUnref\) private\(shadowed,ci,ca\)$|(.*\\$[[:space:]])+.*$}}
  // PRT-O-PARLOOP-P:        {{^ *}}#pragma omp target teams num_teams(2){{$}}
  // PRT-O-PARLOOP-P-NEXT:   {
  // PRT-O-PARLOOP-P-NEXT:     int gi;
  // PRT-O-PARLOOP-P-NEXT:     {{__uint128_t gt;|uint64_t gt_hi;$[[:space:]]*  uint64_t gt_lw;}}
  // PRT-O-PARLOOP-P-NEXT:     int *gp;
  // PRT-O-PARLOOP-P-NEXT:     int ga[2];
  // PRT-O-PARLOOP-P-NEXT:     struct S gs;
  // PRT-O-PARLOOP-P-NEXT:     union U gu;
  // PRT-O-PARLOOP-P-NEXT:     int gUnref;
  // PRT-O-PARLOOP-P-NEXT:     int li;
  // PRT-O-PARLOOP-P-NEXT:     {{__uint128_t lt;|uint64_t lt_hi;$[[:space:]]*  uint64_t lt_lw;}}
  // PRT-O-PARLOOP-P-NEXT:     int *lp;
  // PRT-O-PARLOOP-P-NEXT:     int la[2];
  // PRT-O-PARLOOP-P-NEXT:     struct S ls;
  // PRT-O-PARLOOP-P-NEXT:     union U lu;
  // PRT-O-PARLOOP-P-NEXT:     int lUnref;
  // PRT-O-PARLOOP-P-NEXT:     int shadowed;
                               // The test omits const when testing private.
  // PRT-O-PARLOOP-P-NEXT:     int ci;
  // PRT-O-PARLOOP-P-NEXT:     int ca[3];
  // PRT-OA-PARLOOP-P-NOT:   #pragma
  // PRT-OA-PARLOOP-P:       {{^ *}}// #pragma acc parallel {{LOOP|loop seq}} num_gangs(2) {{private\(gi,gt(_hi,gt_lw)?,gp,ga,gs,gu,gUnref\) private\(li,lt(_hi,lt_lw)?,lp,la,ls,lu,lUnref\) private\(shadowed,ci,ca\)$|(.*\\$[[:space:]])+.*$}}
  //
  // PRT-NOT:      #pragma
  #pragma acc parallel LOOP num_gangs(2)                         \
    CLAUSE(gi,LIST_UINT128(gt),gp, ga,gs , gu , gUnref )         \
    CLAUSE(  li,LIST_UINT128(lt),lp  ,  la  ,ls,  lu , lUnref  ) \
    CLAUSE(shadowed, ci, ca)
  // PRT-PARLOOP-NEXT: {{for (.*) [{]|FORLOOP_HEAD}}
  // PRT-PAR-NEXT: {{(FORLOOP_HEAD$[[:space:]])? *}}{
  FORLOOP_HEAD
  {
    // Read before writing in order to exercise SemaExpr.cpp's
    // isVariableAlreadyCapturedInScopeInfo's code that suppresses adding const
    // to the captured variable in the case of OpenACC.
    int giOld = gi;
    DEF_UINT128_COPY(gtOld, gt);
    // Check a decl with no init: it used to break our transform code.
    const int *gpOld;
    gpOld = gp;
    int gaOld[2] = {ga[0], ga[1]};
    struct S gsOld = gs;
    union U guOld = gu;

    int liOld = li;
    DEF_UINT128_COPY(ltOld, lt);
    const int *lpOld = lp;
    int laOld[2] = {la[0], la[1]};
    struct S lsOld = ls;
    union U luOld = lu;

    int tmp = shadowed;
    int shadowed = tmp + 111;

    gi = 56;
    ASSIGN_UINT128(gt, 0xf08234, 0xa07de1);
    gp = &li;
    ga[0] = 101;
    ga[1] = 202;
    gs.i = 42;
    gs.j = 1;
    gu.i = 13;

    li = 98;
    ASSIGN_UINT128(lt, 0x79ca, 0x2961);
    lp = &li;
    la[0] = 55;
    la[1] = 66;
    ls.i = 333;
    ls.j = 444;
    lu.i = 279;

    shadowed += 111;

#if MODE == MODE_P
    // don't dereference uninitialized pointers
    gpOld = gp;
    lpOld = lp;
#endif
    // PRT:      printf("inside :{{([^;]|[[:space:]])*}};
    // PRT-NEXT: printf("{{([^;]|[[:space:]])*}};
    //
    // It cannot all be in one printf, or nvptx64 output becomes corrupt, so
    // break it into multiple printfs.  The order of output from different
    // gangs is non-deterministic at the level of printfs, but it's easier to
    // write the FileCheck DAG directives at the level of lines.
    //
    // EXE-IF-DAG:  inside : gi:55->56,
    // EXE-CCi-DAG: inside : gi:{{55|56}}->56,
    // EXE-CoP-DAG: inside : gi:{{.+}}->56,
    // EXE-IF-DAG:           gt:[1400,59]->[f08234,a07de1],
    // EXE-CCi-DAG:          gt:[{{1400|f08234}},{{59|a07de1}}]->[f08234,a07de1],
    // EXE-CoP-DAG:          gt:[{{.+}}]->[f08234,a07de1],
    // EXE-IF-DAG:           *gp:{{55->98|9999->9999}},
    // EXE-CCi-DAG:          *gp:{{(56|98)->98|9999->9999}},
    // EXE-CoP-DAG:          *gp:{{.+->98|9999->9999}},
    // EXE-ICCi-DAG:         ga:[{{100|101}},{{200|202}}]->[101,202],
    // EXE-F-DAG:            ga:[100,200]->[101,202],
    // EXE-CoP-DAG:          ga:[{{.+,.+}}]->[101,202],
    // EXE-ICCi-DAG:         gs:[{{33|42}},{{11|1}}]->[42,1],
    // EXE-F-DAG:            gs:[33,11]->[42,1],
    // EXE-CoP-DAG:          gs:[{{.+,.+}}]->[42,1],
    // EXE-ICCi-DAG:         gu.i:{{22|13}}->13,
    // EXE-F-DAG:            gu.i:22->13,
    // EXE-CoP-DAG:          gu.i:{{.+}}->13,
    // EXE-IF-DAG:           li:99->98,
    // EXE-CCi-DAG:          li:{{99|98}}->98,
    // EXE-CoP-DAG:          li:{{.+}}->98,
    // EXE-IF-DAG:           lt:[7a1,62b0]->[79ca,2961],
    // EXE-CCi-DAG:          lt:[{{7a1|79ca}},{{62b0|2961}}]->[79ca,2961],
    // EXE-CoP-DAG:          lt:[{{.+}}]->[79ca,2961],
    // EXE-IF-DAG:           *lp:{{55->98|9999->9999}},
    // EXE-CCi-DAG:          *lp:{{(56|98)->98|9999->9999}},
    // EXE-CoP-DAG:          *lp:{{.+->98|9999->9999}},
    // EXE-ICCi-DAG:         la:[{{77|55}},{{88|66}}]->[55,66],
    // EXE-F-DAG:            la:[77,88]->[55,66],
    // EXE-CoP-DAG:          la:[{{.+,.+}}]->[55,66],
    // EXE-ICCi-DAG:         ls:[{{222|333}},{{111|444}}]->[333,444],
    // EXE-F-DAG:            ls:[222,111]->[333,444],
    // EXE-CoP-DAG:          ls:[{{.+,.+}}]->[333,444],
    // EXE-ICCi-DAG:         lu.i:{{167|279}}->279,
    // EXE-F-DAG:            lu.i:167->279,
    // EXE-CoP-DAG:          lu.i:{{.+}}->279,
    // EXE-DAG:              shadowed:
    // EXE-ICCiF-DAG:        ci:53,
    // EXE-CoP-DAG:          ci:
    // EXE-ICCiF-DAG:        ca:[10,11,12]
    // EXE-CoP-DAG:          ca:[{{.+}},{{.+}},{{.+}}]
    //
    // Duplicate that EXE block exactly.
    //
    // EXE-IF-DAG:  inside : gi:55->56,
    // EXE-CCi-DAG: inside : gi:{{55|56}}->56,
    // EXE-CoP-DAG: inside : gi:{{.+}}->56,
    // EXE-IF-DAG:           gt:[1400,59]->[f08234,a07de1],
    // EXE-CCi-DAG:          gt:[{{1400|f08234}},{{59|a07de1}}]->[f08234,a07de1],
    // EXE-CoP-DAG:          gt:[{{.+}}]->[f08234,a07de1],
    // EXE-IF-DAG:           *gp:{{55->98|9999->9999}},
    // EXE-CCi-DAG:          *gp:{{(56|98)->98|9999->9999}},
    // EXE-CoP-DAG:          *gp:{{.+->98|9999->9999}},
    // EXE-ICCi-DAG:         ga:[{{100|101}},{{200|202}}]->[101,202],
    // EXE-F-DAG:            ga:[100,200]->[101,202],
    // EXE-CoP-DAG:          ga:[{{.+,.+}}]->[101,202],
    // EXE-ICCi-DAG:         gs:[{{33|42}},{{11|1}}]->[42,1],
    // EXE-F-DAG:            gs:[33,11]->[42,1],
    // EXE-CoP-DAG:          gs:[{{.+,.+}}]->[42,1],
    // EXE-ICCi-DAG:         gu.i:{{22|13}}->13,
    // EXE-F-DAG:            gu.i:22->13,
    // EXE-CoP-DAG:          gu.i:{{.+}}->13,
    // EXE-IF-DAG:           li:99->98,
    // EXE-CCi-DAG:          li:{{99|98}}->98,
    // EXE-CoP-DAG:          li:{{.+}}->98,
    // EXE-IF-DAG:           lt:[7a1,62b0]->[79ca,2961],
    // EXE-CCi-DAG:          lt:[{{7a1|79ca}},{{62b0|2961}}]->[79ca,2961],
    // EXE-CoP-DAG:          lt:[{{.+}}]->[79ca,2961],
    // EXE-IF-DAG:           *lp:{{55->98|9999->9999}},
    // EXE-CCi-DAG:          *lp:{{(56|98)->98|9999->9999}},
    // EXE-CoP-DAG:          *lp:{{.+->98|9999->9999}},
    // EXE-ICCi-DAG:         la:[{{77|55}},{{88|66}}]->[55,66],
    // EXE-F-DAG:            la:[77,88]->[55,66],
    // EXE-CoP-DAG:          la:[{{.+,.+}}]->[55,66],
    // EXE-ICCi-DAG:         ls:[{{222|333}},{{111|444}}]->[333,444],
    // EXE-F-DAG:            ls:[222,111]->[333,444],
    // EXE-CoP-DAG:          ls:[{{.+,.+}}]->[333,444],
    // EXE-ICCi-DAG:         lu.i:{{167|279}}->279,
    // EXE-F-DAG:            lu.i:167->279,
    // EXE-CoP-DAG:          lu.i:{{.+}}->279,
    // EXE-DAG:              shadowed:
    // EXE-ICCiF-DAG:        ci:53,
    // EXE-CoP-DAG:          ci:
    // EXE-ICCiF-DAG:        ca:[10,11,12]
    // EXE-CoP-DAG:          ca:[{{.+}},{{.+}},{{.+}}]
    printf("inside : gi:%d->%d,\n"
           "         gt:[%lx,%lx]->[%lx,%lx],\n"
           "         *gp:%d->%d,\n"
           "         ga:[%d,%d]->[%d,%d],\n"
           "         gs:[%d,%d]->[%d,%d],\n"
           "         gu.i:%d->%d,\n"
           "         li:%d->%d,\n"
           "         lt:[%lx,%lx]->[%lx,%lx],\n",
           giOld, gi,
           HI_UINT128(gtOld), LW_UINT128(gtOld),
           HI_UINT128(gt), LW_UINT128(gt),
           DEREF_PTR(gpOld), DEREF_PTR(gp),
           gaOld[0], gaOld[1], ga[0], ga[1],
           gsOld.i, gsOld.j, gs.i, gs.j,
           guOld.i, gu.i,
           liOld, li,
           HI_UINT128(ltOld), LW_UINT128(ltOld),
           HI_UINT128(lt), LW_UINT128(lt));
    printf("         *lp:%d->%d,\n"
           "         la:[%d,%d]->[%d,%d],\n"
           "         ls:[%d,%d]->[%d,%d],\n"
           "         lu.i:%d->%d,\n"
           "         shadowed:%d,\n"
           "         ci:%d,\n"
           "         ca:[%d,%d,%d]\n",
           DEREF_PTR(lpOld), DEREF_PTR(lp),
           laOld[0], laOld[1], la[0], la[1],
           lsOld.i, lsOld.j, ls.i, ls.j,
           luOld.i, lu.i,
           shadowed,
           ci, ca[0],
           ca[1], ca[2]);
  } // PRT-NEXT: }

  // DMP: CallExpr
  //
  // EXE-ICiFP:      outside: gi:55->55,
  // EXE-CCo:        outside: gi:55->56,
  // EXE-ICiFP-NEXT:          gt:[1400,59]->[1400,59],
  // EXE-CCo-NEXT:            gt:[1400,59]->[f08234,a07de1],
  // EXE-ICiFP-NEXT:          *gp:55->55,
  // EXE-CCo-NEXT:            *gp:56->{{98|9999}},
  // EXE-ICCo-NEXT:           ga:[100,200]->[101,202],
  // EXE-CiFP-NEXT:           ga:[100,200]->[100,200],
  // EXE-ICCo-NEXT:           gs:[33,11]->[42,1],
  // EXE-CiFP-NEXT:           gs:[33,11]->[33,11],
  // EXE-ICCo-NEXT:           gu.i:22->13,
  // EXE-CiFP-NEXT:           gu.i:22->22,
  // EXE-NEXT:                gUnref:2->2,
  // EXE-ICiFP-NEXT:          li:99->99,
  // EXE-CCo-NEXT:            li:99->98,
  // EXE-ICiFP-NEXT:          lt:[7a1,62b0]->[7a1,62b0],
  // EXE-CCo-NEXT:            lt:[7a1,62b0]->[79ca,2961]
  // EXE-ICiFP-NEXT:          *lp:55->55,
  // EXE-CCo-NEXT:            *lp:56->{{98|9999}},
  // EXE-ICCo-NEXT:           la:[77,88]->[55,66],
  // EXE-CiFP-NEXT:           la:[77,88]->[77,88],
  // EXE-ICCo-NEXT:           ls:[222,111]->[333,444],
  // EXE-CiFP-NEXT:           ls:[222,111]->[222,111],
  // EXE-ICCo-NEXT:           lu.i:167->279,
  // EXE-CiFP-NEXT:           lu.i:167->167,
  // EXE-NEXT:                lUnref:9->9,
  // EXE-ICCiFP-NEXT:         shadowed:111,
  // EXE-Co-NEXT:             shadowed:
  // EXE-ICCiFP-NEXT:         ci:53,
  // EXE-Co-NEXT:             ci:
  // EXE-ICCiFP-NEXT:         ca:[10,11,12]
  // EXE-Co-NEXT:             ca:[{{.*}},{{.*}},{{.*}}]
  printf("outside: gi:%d->%d,\n"
         "         gt:[%lx,%lx]->[%lx,%lx],\n"
         "         *gp:%d->%d,\n"
         "         ga:[%d,%d]->[%d,%d],\n"
         "         gs:[%d,%d]->[%d,%d],\n"
         "         gu.i:%d->%d,\n"
         "         gUnref:%d->%d,\n"
         "         li:%d->%d,\n"
         "         lt:[%lx,%lx]->[%lx,%lx],\n"
         "         *lp:%d->%d,\n"
         "         la:[%d,%d]->[%d,%d],\n"
         "         ls:[%d,%d]->[%d,%d],\n"
         "         lu.i:%d->%d,\n"
         "         lUnref:%d->%d,\n"
         "         shadowed:%d,\n"
         "         ci:%d,\n"
         "         ca:[%d,%d,%d]\n",
         giOld, gi,
         HI_UINT128(gtOld), LW_UINT128(gtOld),
         HI_UINT128(gt), LW_UINT128(gt),
         *gpOld,
#if MODE != MODE_C0  && MODE != MODE_C1  && MODE != MODE_C2 && \
    MODE != MODE_Co0 && MODE != MODE_Co1 && MODE != MODE_Co2
         *gp,
#else
         DEREF_PTR(gp),
#endif
         gaOld[0], gaOld[1], ga[0], ga[1],
         gsOld.i, gsOld.j, gs.i, gs.j,
         guOld.i, gu.i,
         gUnrefOld, gUnref,
         liOld, li,
         HI_UINT128(ltOld), LW_UINT128(ltOld),
         HI_UINT128(lt), LW_UINT128(lt),
         *lpOld,
#if MODE != MODE_C0  && MODE != MODE_C1  && MODE != MODE_C2 && \
    MODE != MODE_Co0 && MODE != MODE_Co1 && MODE != MODE_Co2
         *lp,
#else
         DEREF_PTR(lp),
#endif
         laOld[0], laOld[1], la[0], la[1],
         lsOld.i, lsOld.j, ls.i, ls.j,
         luOld.i, lu.i,
         lUnrefOld, lUnref,
         shadowed,
         ci, ca[0],
         ca[1], ca[2]);

  return 0;
}
// EXE-NOT:{{.}}

