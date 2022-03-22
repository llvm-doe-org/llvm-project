// Check correct loop partitioning and implicit data attributes on
// "acc loop" within "acc parallel" or on effective "acc loop" from
// "acc parallel loop".
//
// When independent and gang are omitted, they're usually implicit.  Thus, test
// code is almost identical across these cases, so it's easier to parameterize
// the test code instead of repeat it entirely with minor differences.
// RUN: %data extra-clauses {
// RUN:   (independent=independent gang=gang fc-pres=IND,GANG      )
// RUN:   (independent=            gang=gang fc-pres=NO-IND,GANG   )
// RUN:   (independent=independent gang=     fc-pres=IND,NO-GANG   )
// RUN:   (independent=            gang=     fc-pres=NO-IND,NO-GANG)
// RUN: }
// RUN: %for extra-clauses {
// RUN:   %acc-check-dmp{                                                      \
// RUN:     clang-args: -DINDEPENDENT=%[independent] -DGANG=%[gang];           \
// RUN:     fc-args:    ;                                                      \
// RUN:     fc-pres:    %[fc-pres]}
// RUN:   %acc-check-prt{                                                      \
// RUN:     clang-args: -DINDEPENDENT=%[independent] -DGANG=%[gang];           \
// RUN:     fc-args:    ;                                                      \
// RUN:     fc-pres:    %[fc-pres]}
// RUN:   %acc-check-exe{                                                      \
// RUN:     clang-args: -DINDEPENDENT=%[independent] -DGANG=%[gang];           \
// RUN:     exe-args:   ;                                                      \
// RUN:     fc-args:    ;                                                      \
// RUN:     fc-pres:    %[fc-pres]}
// RUN: }

// END.

/* expected-error 0 {{}} */

// FIXME: Clang produces spurious warning diagnostics for nvptx64 offload.  This
// issue is not limited to Clacc and is present upstream:
/* nvptx64-warning@*:* 0+ {{Linking two modules of different target triples}} */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if TGT_USE_STDIO
# define TGT_PRINTF(...) printf(__VA_ARGS__)
#else
# define TGT_PRINTF(...)
#endif

//==============================================================================
// The functions below encapsulate execution checks that variable accesses
// behave correctly for various loop partitioning and implicit data attribute
// scenarios.  They by no means fully check semantics, which is challenging
// given parallel execution, but they are better than not having execution
// checks at all.
//
// These functions assume Var is reassigned from OldVal to NewVal after the
// function is called in every iteration of the enclosing loop, which might
// execute in one of various redundant or partitioned modes.  To check Var's
// value, each function reads Var's value only once (at the function call
// expanded from each corresponding macro).  If it were to instead read Var
// before comparing with OldVal and again before comparing with NewVal, another
// thread might reassign Var from OldVal to NewVal in between the reads, and the
// function would then falsely report that Var had an unexpected value.
//==============================================================================

// Check the values that a loop encounters for Var such that every gang's
// iteration through the loop is complete, is sequential, and has private access
// to Var.  Thus, every call at LoopIdx=0 sees OldVal, and every call at
// LoopIdx>0 sees NewVal.
#define checkLoopSeqPrivate(Result, LoopIdx, Var, OldVal, NewVal)              \
  checkLoopSeqPrivate_(&((Result).Var ## AtI0FoundBadVal),                     \
                       &((Result).Var ## AtI1FoundBadVal),                     \
                       (LoopIdx), Var, (OldVal), (NewVal))
static void checkLoopSeqPrivate_(bool *VarAtI0FoundBadVal,
                                 bool *VarAtI1FoundBadVal, int LoopIdx,
                                 int CurVal, int OldVal, int NewVal) {
  if (LoopIdx == 0) {
    if (CurVal != OldVal)
      *VarAtI0FoundBadVal = true;
  } else if (CurVal != NewVal) {
    *VarAtI1FoundBadVal = true;
  }
}

// Check the values that a loop encounters for Var such that every gang's
// iteration through the loop is complete, is sequential, and potentially shares
// parallel access to Var.  Thus, at least one call sees OldVal at LoopIdx=0,
// some calls at LoopIdx=0 might see NewVal, and every call at LoopIdx>0 sees
// NewVal.
#define checkLoopSeqShared(Result, LoopIdx, Var, OldVal, NewVal)               \
  checkLoopSeqShared_(&((Result).Var ## FoundOldVal),                          \
                      &((Result).Var ## FoundBadVal),                          \
                      (LoopIdx), Var[0], (OldVal), (NewVal))
static void checkLoopSeqShared_(bool *VarFoundOldVal, bool *VarFoundBadVal,
                                int LoopIdx, int CurVal, int OldVal,
                                int NewVal) {
  if (CurVal == OldVal && LoopIdx == 0)
    *VarFoundOldVal = true;
  else if (CurVal != NewVal)
    *VarFoundBadVal = true;
}

// Check the values that a loop encounters for Var such that every gang's
// iteration through the loop is potentially for just a loop partition, is
// sequential (at least in our implementation even if not guaranteed by
// OpenACC), and has private access to Var.  Thus, every call at LoopIdx=0
// sees OldVal, and every call at LoopIdx>0 sees either OldVal or NewVal.
#define checkLoopPartPrivate(Result, LoopIdx, Var, OldVal, NewVal)             \
  checkLoopPartPrivate_(&((Result).Var ## AtI0FoundBadVal),                    \
                        &((Result).Var ## AtI1FoundBadVal),                    \
                        (LoopIdx), Var, (OldVal), (NewVal))
static void checkLoopPartPrivate_(bool *VarAtI0FoundBadVal,
                                  bool *VarAtI1FoundBadVal, int LoopIdx,
                                  int CurVal, int OldVal, int NewVal) {
  if (LoopIdx == 0) {
    if (CurVal != OldVal)
      *VarAtI0FoundBadVal = true;
  } else if (CurVal != OldVal && CurVal != NewVal) {
    *VarAtI1FoundBadVal = true;
  }
}

// Check the values that a loop encounters for Var such that every
// gang/worker/vector's iteration through the loop is potentially for just a
// loop partition, is sequential (at least in our implementation even if not
// guaranteed by OpenACC), and potentially shares parallel access to Var.  Thus,
// at least one call sees OldVal, and every calls sees either OldVal or NewVal.
#define checkLoopPartShared(Result, LoopIdx, VarName, VarVal, OldVal, NewVal)  \
  checkLoopPartShared_(&((Result).VarName ## FoundOldVal),                     \
                       &((Result).VarName ## FoundBadVal),                     \
                       (LoopIdx), VarVal, (OldVal), (NewVal))
static void checkLoopPartShared_(bool *VarFoundOldVal, bool *VarFoundBadVal,
                                 int LoopIdx, int CurVal, int OldVal,
                                 int NewVal) {
  if (CurVal == OldVal)
    *VarFoundOldVal = true;
  else if (CurVal != NewVal)
    *VarFoundBadVal = true;
}

//==============================================================================
// Check within parallel constructs.
//==============================================================================

int main() {
  //----------------------------------------------------------------------------
  // For separate acc parallel and acc loop, check:
  // - Declared loop control variable, i.
  // - Assigned loop control variable, k, predetermined private when the loop is
  //   partitioned, but implicitly shared when loop is sequential.
  // - Inner loop control variables, j and l, each not handled as the control
  //   variable for the enclosing acc loop and so not predetermined private.
  //   For collapse values greater than 1, this is checked in loop-collapse.c.
  // - Gang-private (because locally declared) variables, j and l, accessed in
  //   the acc loops and the acc parallel.
  // - Implicitly gang-shared variable (save) accessed in the acc loop and the
  //   acc parallel.
  // - Implicitly gang-private variable (loopOnly) and implicitly gang-shared
  //   variable (loopOnlyArr) accessed in the acc loop but not in the acc
  //   parallel.
  // - All combinations of auto, independent, seq, gang, worker, and vector.
  //----------------------------------------------------------------------------

  //............................................................................
  // Sequential loops.
  //............................................................................

  {
    int loopOnly;
    int loopOnlyArr[1];
    struct {
      bool loopOnlyAtI0FoundBadVal;
      bool loopOnlyAtI1FoundBadVal;

      bool loopOnlyArrFoundOldVal;
      bool loopOnlyArrFoundBadVal;

      bool declNestedLoopFoundBadVal;

      bool assignLoopFoundBadVal;
      bool assignNestedLoopFoundBadVal;
      bool writeInLaterLoopFoundBadVal;
    } save;

    //. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 
    // acc loop seq
    //. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 

    // DMP-LABEL: StringLiteral {{.*}} "acc loop seq\n"
    // PRT-LABEL: printf("acc loop seq\n");
    //   EXE-NOT: {{.}}
    // EXE-LABEL: acc loop seq{{$}}
    //   EXE-NOT: {{.}}
    memset(&save, 0, sizeof(save));
    loopOnly = 88;
    loopOnlyArr[0] = 88;
    printf("acc loop seq\n");

    //      DMP: ACCParallelDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 3
    // DMP-NEXT:   ACCNomapClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnly' 'int'
    // DMP-NEXT:   ACCCopyClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'save'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'save'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    // DMP-NEXT:   ACCFirstprivateClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnly' 'int'
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 3
    // DMP-NEXT:     OMPMapClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'save'
    // DMP-NEXT:       DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    // DMP-NEXT:     OMPSharedClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'save'
    // DMP-NEXT:       DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    // DMP-NEXT:     OMPFirstprivateClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'loopOnly' 'int'
    //      DMP:     CompoundStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc parallel num_gangs(3){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(3) map(ompx_hold,tofrom: save,loopOnlyArr) shared(save,loopOnlyArr) firstprivate(loopOnly){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(3) map(ompx_hold,tofrom: save,loopOnlyArr) shared(save,loopOnlyArr) firstprivate(loopOnly){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(3){{$}}
    //    PRT-NEXT: {
    #pragma acc parallel num_gangs(3)
    {
      // PRT-NEXT: int j = 0;
      int j = 0;
      //      DMP: ACCLoopDirective
      // DMP-NEXT:   ACCSeqClause
      //  DMP-NOT:     <implicit>
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'j' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'save'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnly' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
      // DMP-NEXT:   impl: ForStmt
      //
      //  PRT-A-NEXT: {{^ *}}#pragma acc loop seq
      // PRT-AO-SAME: {{^ }}// discarded in OpenMP translation
      //  PRT-A-SAME: {{^$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop seq // discarded in OpenMP translation{{$}}
      //    PRT-NEXT: for ({{.*}}) {
      //    PRT-NEXT:   {{printf|TGT_PRINTF}}
      //    PRT-NEXT:   for (j ={{.*}})
      //    PRT-NEXT:     ;
      //         PRT: }
      //
      // Gang-redundant mode executes each loop iteration per gang.
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      #pragma acc loop seq
      for (int i = 0; i < 2; ++i) {
        TGT_PRINTF("loop iteration\n");
        for (j = 0; j < 2; ++j)
          ;
        checkLoopSeqPrivate(save, i, loopOnly, 88, 77);
        loopOnly = 77;
        checkLoopSeqShared(save, i, loopOnlyArr, 88, 77);
        loopOnlyArr[0] = 77;
      }
      // Each gang will see the final j from the j loop above.
      if (j != 2)
        save.declNestedLoopFoundBadVal = true;

      //      PRT: int k = 999;
      // PRT-NEXT: int l = 888;
      int k = 999;
      int l = 888;
      //      DMP: ACCLoopDirective
      // DMP-NEXT:   ACCSeqClause
      //  DMP-NOT:     <implicit>
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'l' 'int'
      // DMP-NEXT:   impl: ForStmt
      //
      //  PRT-A-NEXT: {{^ *}}#pragma acc loop seq
      // PRT-AO-SAME: {{^ }}// discarded in OpenMP translation
      // PRT-AO-SAME: {{^$}}
      // PRT-OA-NEXT: // #pragma acc loop seq // discarded in OpenMP translation{{$}}
      //    PRT-NEXT: for (k ={{.*}}) {
      //    PRT-NEXT:   {{printf|TGT_PRINTF}}
      //    PRT-NEXT:   for (l ={{.*}})
      //    PRT-NEXT:     ;
      //    PRT-NEXT: }
      //
      // Gang-redundant mode executes each loop iteration per gang.
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      #pragma acc loop seq
      for (k = 1; k < 3; ++k) {
        TGT_PRINTF("loop iteration\n");
        for (l = 0; l < 2; ++l)
          ;
      }
      // Each gang will see the final k and l from the loop above.
      if (k != 3)
        save.assignLoopFoundBadVal = true;
      if (l != 2)
        save.assignNestedLoopFoundBadVal = true;
    } // PRT: }

    // EXE-NEXT: save.loopOnlyAtI0FoundBadVal=0
    // EXE-NEXT: save.loopOnlyAtI1FoundBadVal=0
    // EXE-NEXT: save.loopOnlyArrFoundOldVal=1
    // EXE-NEXT: save.loopOnlyArrFoundBadVal=0
    // EXE-NEXT: save.declNestedLoopFoundBadVal=0
    // EXE-NEXT: save.assignLoopFoundBadVal=0
    // EXE-NEXT: save.assignNestedLoopFoundBadVal=0
    // EXE-NEXT: loopOnly=88
    // EXE-NEXT: loopOnlyArr[0]=77
    printf("save.loopOnlyAtI0FoundBadVal=%d\n", save.loopOnlyAtI0FoundBadVal);
    printf("save.loopOnlyAtI1FoundBadVal=%d\n", save.loopOnlyAtI1FoundBadVal);
    printf("save.loopOnlyArrFoundOldVal=%d\n", save.loopOnlyArrFoundOldVal);
    printf("save.loopOnlyArrFoundBadVal=%d\n", save.loopOnlyArrFoundBadVal);
    printf("save.declNestedLoopFoundBadVal=%d\n", save.declNestedLoopFoundBadVal);
    printf("save.assignLoopFoundBadVal=%d\n", save.assignLoopFoundBadVal);
    printf("save.assignNestedLoopFoundBadVal=%d\n", save.assignNestedLoopFoundBadVal);
    printf("loopOnly=%d\n", loopOnly);
    printf("loopOnlyArr[0]=%d\n", loopOnlyArr[0]);

    //. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 
    // acc loop auto GANG
    //. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 

    // DMP-LABEL: StringLiteral {{.*}} "acc loop auto GANG\n"
    // PRT-LABEL: printf("acc loop auto GANG\n");
    //   EXE-NOT: {{.}}
    // EXE-LABEL: acc loop auto GANG{{$}}
    //   EXE-NOT: {{.}}
    memset(&save, 0, sizeof(save));
    loopOnly = 88;
    loopOnlyArr[0] = 88;
    printf("acc loop auto GANG\n");

    //           DMP: ACCParallelDirective
    //      DMP-NEXT:   ACCNumGangsClause
    //      DMP-NEXT:     IntegerLiteral {{.*}} 'int' 3
    //      DMP-NEXT:   ACCNomapClause {{.*}} <implicit>
    //      DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnly' 'int'
    //      DMP-NEXT:   ACCCopyClause {{.*}} <implicit>
    //      DMP-NEXT:     DeclRefExpr {{.*}} 'save'
    //      DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    //      DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    //      DMP-NEXT:     DeclRefExpr {{.*}} 'save'
    //      DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    //      DMP-NEXT:   ACCFirstprivateClause {{.*}} <implicit>
    //      DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnly' 'int'
    //      DMP-NEXT:   impl: OMPTargetTeamsDirective
    //      DMP-NEXT:     OMPNum_teamsClause
    //      DMP-NEXT:       IntegerLiteral {{.*}} 'int' 3
    //      DMP-NEXT:     OMPMapClause
    //       DMP-NOT:       <implicit>
    //      DMP-NEXT:       DeclRefExpr {{.*}} 'save'
    //      DMP-NEXT:       DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    //      DMP-NEXT:     OMPSharedClause
    //       DMP-NOT:       <implicit>
    //      DMP-NEXT:       DeclRefExpr {{.*}} 'save'
    //      DMP-NEXT:       DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    //      DMP-NEXT:     OMPFirstprivateClause
    //       DMP-NOT:       <implicit>
    //      DMP-NEXT:       DeclRefExpr {{.*}} 'loopOnly' 'int'
    //           DMP:     CompoundStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc parallel num_gangs(3){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(3) map(ompx_hold,tofrom: save,loopOnlyArr) shared(save,loopOnlyArr) firstprivate(loopOnly){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(3) map(ompx_hold,tofrom: save,loopOnlyArr) shared(save,loopOnlyArr) firstprivate(loopOnly){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(3){{$}}
    //    PRT-NEXT: {
    #pragma acc parallel num_gangs(3)
    {
      // PRT-NEXT: int j = 0;
      int j = 0;
      //           DMP: ACCLoopDirective
      //      DMP-NEXT:   ACCAutoClause
      //       DMP-NOT:     <implicit>
      // DMP-GANG-NEXT:   ACCGangClause
      //  DMP-GANG-NOT:     <implicit>
      //      DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      //      DMP-NEXT:     DeclRefExpr {{.*}} 'j' 'int'
      //      DMP-NEXT:     DeclRefExpr {{.*}} 'save'
      //      DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnly' 'int'
      //      DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
      //      DMP-NEXT:   impl: ForStmt
      //
      //           PRT-A-NEXT: {{^ *}}#pragma acc loop auto
      //       PRT-A-SRC-SAME: {{^ }}GANG
      //  PRT-A-AST-GANG-SAME: {{^ }}gang
      //          PRT-AO-SAME: {{^ }}// discarded in OpenMP translation
      //           PRT-A-SAME: {{^$}}
      //          PRT-OA-NEXT: {{^ *}}// #pragma acc loop auto
      //      PRT-OA-SRC-SAME: {{^ }}GANG
      // PRT-OA-AST-GANG-SAME: {{^ }}gang
      //          PRT-OA-SAME: {{^ }}// discarded in OpenMP translation{{$}}
      //             PRT-NEXT: for ({{.*}}) {
      //             PRT-NEXT:   {{printf|TGT_PRINTF}}
      //             PRT-NEXT:   for (j ={{.*}})
      //             PRT-NEXT:     ;
      //                  PRT: }
      //
      // Gang-redundant mode executes each loop iteration per gang.
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      #pragma acc loop auto GANG
      for (int i = 0; i < 2; ++i) {
        TGT_PRINTF("loop iteration\n");
        for (j = 0; j < 2; ++j)
          ;
        checkLoopSeqPrivate(save, i, loopOnly, 88, 77);
        loopOnly = 77;
        checkLoopSeqShared(save, i, loopOnlyArr, 88, 77);
        loopOnlyArr[0] = 77;
      }
      // Each gang will see the final j from the j loop above.
      if (j != 2)
        save.declNestedLoopFoundBadVal = true;

      //      PRT: int k = 999;
      // PRT-NEXT: int l = 888;
      int k = 999;
      int l = 888;
      //           DMP: ACCLoopDirective
      //      DMP-NEXT:   ACCAutoClause
      //       DMP-NOT:     <implicit>
      // DMP-GANG-NEXT:   ACCGangClause
      //  DMP-GANG-NOT:     <implicit>
      //      DMP-NEXT:   ACCPrivateClause {{.*}} <predetermined>
      //      DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
      //      DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      //      DMP-NEXT:     DeclRefExpr {{.*}} 'l' 'int'
      //      DMP-NEXT:   impl: CompoundStmt
      //      DMP-NEXT:     DeclStmt
      //      DMP-NEXT:       VarDecl {{.*}} k 'int'
      //      DMP-NEXT:     ForStmt
      //
      //          PRT-AO-NEXT: // v----------ACC----------v
      //           PRT-A-NEXT: {{^ *}}#pragma acc loop auto
      //       PRT-A-SRC-SAME: {{^ }}GANG
      //  PRT-A-AST-GANG-SAME: {{^ }}gang
      //           PRT-A-SAME: {{^$}}
      //           PRT-A-NEXT: for (k ={{.*}}) {
      //           PRT-A-NEXT:   {{print|TGT_PRINTF}}
      //           PRT-A-NEXT:   for (l ={{.*}})
      //           PRT-A-NEXT:     ;
      //           PRT-A-NEXT: }
      //          PRT-AO-NEXT: // ---------ACC->OMP--------
      //          PRT-AO-NEXT: // {
      //          PRT-AO-NEXT: //   int k;
      //          PRT-AO-NEXT: //   for (k ={{.*}}) {
      //          PRT-AO-NEXT: //     {{printf|TGT_PRINTF}}
      //          PRT-AO-NEXT: //     for (l ={{.*}})
      //          PRT-AO-NEXT: //       ;
      //          PRT-AO-NEXT: //   }
      //          PRT-AO-NEXT: // }
      //          PRT-AO-NEXT: // ^----------OMP----------^
      //
      //          PRT-OA-NEXT: // v----------OMP----------v
      //           PRT-O-NEXT: {
      //           PRT-O-NEXT:   int k;
      //           PRT-O-NEXT:   for (k ={{.*}}) {
      //           PRT-O-NEXT:     {{printf|TGT_PRINTF}}
      //           PRT-O-NEXT:     for (l ={{.*}})
      //           PRT-O-NEXT:       ;
      //           PRT-O-NEXT:   }
      //           PRT-O-NEXT: }
      //          PRT-OA-NEXT: // ---------OMP<-ACC--------
      //          PRT-OA-NEXT: {{^ *}}// #pragma acc loop auto
      //      PRT-OA-SRC-SAME: {{^ }}GANG
      // PRT-OA-AST-GANG-SAME: {{^ }}gang
      //          PRT-OA-SAME: {{^$}}
      //          PRT-OA-NEXT: // for (k ={{.*}}) {
      //          PRT-OA-NEXT: //   {{printf|TGT_PRINTF}}
      //          PRT-OA-NEXT: //   for (l ={{.*}})
      //          PRT-OA-NEXT: //     ;
      //          PRT-OA-NEXT: // }
      //          PRT-OA-NEXT: // ^----------ACC----------^
      //
      // PRT-NOACC-NEXT: for (k ={{.*}}) {
      // PRT-NOACC-NEXT:   {{printf|TGT_PRINTF}}
      // PRT-NOACC-NEXT:   for (l ={{.*}})
      // PRT-NOACC-NEXT:     ;
      // PRT-NOACC-NEXT: }
      //
      // Gang-redundant mode executes each loop iteration per gang.
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      #pragma acc loop auto GANG
      for (k = 1; k < 3; ++k) {
        TGT_PRINTF("loop iteration\n");
        for (l = 0; l < 2; ++l)
          ;
      }
      // Each gang will see the old k from before the loop above.
      if (k != 999)
        save.assignLoopFoundBadVal = true;
      // Each gang will see the final l from the loop above.
      if (l != 2)
        save.assignNestedLoopFoundBadVal = true;

      // PRT: k = 999;
      k = 999;
      // k should have been dropped from the loop control variable list, so it
      // should now be implicitly shared in all cases.
      //
      //           DMP: ACCLoopDirective
      //      DMP-NEXT:   ACCAutoClause
      //       DMP-NOT:     <implicit>
      // DMP-GANG-NEXT:   ACCGangClause
      //  DMP-GANG-NOT:     <implicit>
      //      DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      //      DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
      //      DMP-NEXT:   impl: ForStmt
      //
      //           PRT-A-NEXT: {{^ *}}#pragma acc loop auto
      //       PRT-A-SRC-SAME: {{^ }}GANG
      //  PRT-A-AST-GANG-SAME: {{^ }}gang
      //          PRT-AO-SAME: {{^ }}// discarded in OpenMP translation
      //           PRT-A-SAME: {{^$}}
      //          PRT-OA-NEXT: {{^ *}}// #pragma acc loop auto
      //      PRT-OA-SRC-SAME: {{^ }}GANG
      // PRT-OA-AST-GANG-SAME: {{^ }}gang
      //          PRT-OA-SAME: {{^ }}// discarded in OpenMP translation{{$}}
      //             PRT-NEXT: for (int i = {{.*}}) {
      //             PRT-NEXT:   {{printf|TGT_PRINTF}}
      //             PRT-NEXT:   k = 9999;
      //             PRT-NEXT: }
      //
      // Gang-redundant mode executes each loop iteration per gang.
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      #pragma acc loop auto GANG
      for (int i = 0; i < 2; ++i) {
        TGT_PRINTF("loop iteration\n");
        k = 9999;
      }
      // Each gang will see the k from the i loop above.
      if (k != 9999)
        save.writeInLaterLoopFoundBadVal = true;
    }

    // EXE-NEXT: save.loopOnlyAtI0FoundBadVal=0
    // EXE-NEXT: save.loopOnlyAtI1FoundBadVal=0
    // EXE-NEXT: save.loopOnlyArrFoundOldVal=1
    // EXE-NEXT: save.loopOnlyArrFoundBadVal=0
    // EXE-NEXT: save.declNestedLoopFoundBadVal=0
    // EXE-NEXT: save.assignLoopFoundBadVal=0
    // EXE-NEXT: save.assignNestedLoopFoundBadVal=0
    // EXE-NEXT: save.writeInLaterLoopFoundBadVal=0
    // EXE-NEXT: loopOnly=88
    // EXE-NEXT: loopOnlyArr[0]=77
    printf("save.loopOnlyAtI0FoundBadVal=%d\n", save.loopOnlyAtI0FoundBadVal);
    printf("save.loopOnlyAtI1FoundBadVal=%d\n", save.loopOnlyAtI1FoundBadVal);
    printf("save.loopOnlyArrFoundOldVal=%d\n", save.loopOnlyArrFoundOldVal);
    printf("save.loopOnlyArrFoundBadVal=%d\n", save.loopOnlyArrFoundBadVal);
    printf("save.declNestedLoopFoundBadVal=%d\n", save.declNestedLoopFoundBadVal);
    printf("save.assignLoopFoundBadVal=%d\n", save.assignLoopFoundBadVal);
    printf("save.assignNestedLoopFoundBadVal=%d\n", save.assignNestedLoopFoundBadVal);
    printf("save.writeInLaterLoopFoundBadVal=%d\n", save.writeInLaterLoopFoundBadVal);
    printf("loopOnly=%d\n", loopOnly);
    printf("loopOnlyArr[0]=%d\n", loopOnlyArr[0]);

    //. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 
    // acc loop auto GANG worker
    //. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 

    // DMP-LABEL: StringLiteral {{.*}} "acc loop auto GANG worker\n"
    // PRT-LABEL: printf("acc loop auto GANG worker\n");
    //   EXE-NOT: {{.}}
    // EXE-LABEL: acc loop auto GANG worker{{$}}
    //   EXE-NOT: {{.}}
    memset(&save, 0, sizeof(save));
    loopOnly = 88;
    loopOnlyArr[0] = 88;
    printf("acc loop auto GANG worker\n");

    //      DMP: ACCParallelDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 3
    // DMP-NEXT:   ACCNomapClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnly' 'int'
    // DMP-NEXT:   ACCCopyClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'save'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'save'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    // DMP-NEXT:   ACCFirstprivateClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnly' 'int'
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 3
    // DMP-NEXT:     OMPMapClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'save'
    // DMP-NEXT:       DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    // DMP-NEXT:     OMPSharedClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'save'
    // DMP-NEXT:       DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    // DMP-NEXT:     OMPFirstprivateClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'loopOnly' 'int'
    //      DMP:     CompoundStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc parallel num_gangs(3){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(3) map(ompx_hold,tofrom: save,loopOnlyArr) shared(save,loopOnlyArr) firstprivate(loopOnly){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(3) map(ompx_hold,tofrom: save,loopOnlyArr) shared(save,loopOnlyArr) firstprivate(loopOnly){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(3){{$}}
    //    PRT-NEXT: {
    #pragma acc parallel num_gangs(3)
    {
      // PRT-NEXT: int j = 0;
      int j = 0;
      //           DMP: ACCLoopDirective
      //      DMP-NEXT:   ACCAutoClause
      //       DMP-NOT:     <implicit>
      // DMP-GANG-NEXT:   ACCGangClause
      //  DMP-GANG-NOT:     <implicit>
      //      DMP-NEXT:   ACCWorkerClause
      //       DMP-NOT:     <implicit>
      //      DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      //      DMP-NEXT:     DeclRefExpr {{.*}} 'j' 'int'
      //      DMP-NEXT:     DeclRefExpr {{.*}} 'save'
      //      DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnly' 'int'
      //      DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
      //      DMP-NEXT:   impl: ForStmt
      //
      //           PRT-A-NEXT: {{^ *}}#pragma acc loop auto
      //       PRT-A-SRC-SAME: {{^ }}GANG
      //  PRT-A-AST-GANG-SAME: {{^ }}gang
      //           PRT-A-SAME: {{^ }}worker
      //          PRT-AO-SAME: {{^ }}// discarded in OpenMP translation
      //           PRT-A-SAME: {{^$}}
      //          PRT-OA-NEXT: {{^ *}}// #pragma acc loop auto
      //      PRT-OA-SRC-SAME: {{^ }}GANG
      // PRT-OA-AST-GANG-SAME: {{^ }}gang
      //          PRT-OA-SAME: {{^ }}worker
      //          PRT-OA-SAME: {{^ }}// discarded in OpenMP translation{{$}}
      //             PRT-NEXT: for (int i = {{.*}}) {
      //             PRT-NEXT:   {{printf|TGT_PRINTF}}
      //             PRT-NEXT:   for (j ={{.*}})
      //             PRT-NEXT:     ;
      //                  PRT: }
      //
      // Gang-redundant mode executes each loop iteration per gang.
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      #pragma acc loop auto GANG worker
      for (int i = 0; i < 2; ++i) {
        TGT_PRINTF("loop iteration\n");
        for (j = 0; j < 2; ++j)
          ;
        checkLoopSeqPrivate(save, i, loopOnly, 88, 77);
        loopOnly = 77;
        checkLoopSeqShared(save, i, loopOnlyArr, 88, 77);
        loopOnlyArr[0] = 77;
      }
      // Each gang will see the final j from the j loop above.
      if (j != 2)
        save.declNestedLoopFoundBadVal = true;

      //      PRT: int k = 999;
      // PRT-NEXT: int l = 888;
      int k = 999;
      int l = 888;
      //           DMP: ACCLoopDirective
      //      DMP-NEXT:   ACCAutoClause
      //       DMP-NOT:     <implicit>
      // DMP-GANG-NEXT:   ACCGangClause
      //  DMP-GANG-NOT:     <implicit>
      //      DMP-NEXT:   ACCWorkerClause
      //       DMP-NOT:     <implicit>
      //      DMP-NEXT:   ACCPrivateClause {{.*}} <predetermined>
      //      DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
      //      DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      //      DMP-NEXT:     DeclRefExpr {{.*}} 'l' 'int'
      //      DMP-NEXT:   impl: CompoundStmt
      //      DMP-NEXT:     DeclStmt
      //      DMP-NEXT:       VarDecl {{.*}} k 'int'
      //      DMP-NEXT:     ForStmt
      //
      //          PRT-AO-NEXT: // v----------ACC----------v
      //           PRT-A-NEXT: {{^ *}}#pragma acc loop auto
      //       PRT-A-SRC-SAME: {{^ }}GANG
      //  PRT-A-AST-GANG-SAME: {{^ }}gang
      //           PRT-A-SAME: {{^ }}worker{{$}}
      //           PRT-A-NEXT: for (k ={{.*}}) {
      //           PRT-A-NEXT:   {{print|TGT_PRINTF}}
      //           PRT-A-NEXT:   for (l ={{.*}})
      //           PRT-A-NEXT:     ;
      //           PRT-A-NEXT: }
      //          PRT-AO-NEXT: // ---------ACC->OMP--------
      //          PRT-AO-NEXT: // {
      //          PRT-AO-NEXT: //   int k;
      //          PRT-AO-NEXT: //   for (k ={{.*}}) {
      //          PRT-AO-NEXT: //     {{printf|TGT_PRINTF}}
      //          PRT-AO-NEXT: //     for (l ={{.*}})
      //          PRT-AO-NEXT: //       ;
      //          PRT-AO-NEXT: //   }
      //          PRT-AO-NEXT: // }
      //          PRT-AO-NEXT: // ^----------OMP----------^
      //
      //          PRT-OA-NEXT: // v----------OMP----------v
      //           PRT-O-NEXT: {
      //           PRT-O-NEXT:   int k;
      //           PRT-O-NEXT:   for (k ={{.*}}) {
      //           PRT-O-NEXT:     {{printf|TGT_PRINTF}}
      //           PRT-O-NEXT:     for (l ={{.*}})
      //           PRT-O-NEXT:       ;
      //           PRT-O-NEXT:   }
      //           PRT-O-NEXT: }
      //          PRT-OA-NEXT: // ---------OMP<-ACC--------
      //          PRT-OA-NEXT: {{^ *}}// #pragma acc loop auto
      //      PRT-OA-SRC-SAME: {{^ }}GANG
      // PRT-OA-AST-GANG-SAME: {{^ }}gang
      //          PRT-OA-SAME: {{^ }}worker{{$}}
      //          PRT-OA-NEXT: // for (k ={{.*}}) {
      //          PRT-OA-NEXT: //   {{printf|TGT_PRINTF}}
      //          PRT-OA-NEXT: //   for (l ={{.*}})
      //          PRT-OA-NEXT: //     ;
      //          PRT-OA-NEXT: // }
      //          PRT-OA-NEXT: // ^----------ACC----------^
      //
      // PRT-NOACC-NEXT: for (k ={{.*}}) {
      // PRT-NOACC-NEXT:   {{printf|TGT_PRINTF}}
      // PRT-NOACC-NEXT:   for (l ={{.*}})
      // PRT-NOACC-NEXT:     ;
      // PRT-NOACC-NEXT: }
      //
      // Gang-redundant mode executes each loop iteration per gang.
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      #pragma acc loop auto GANG worker
      for (k = 1; k < 3; ++k) {
        TGT_PRINTF("loop iteration\n");
        for (l = 0; l < 2; ++l)
          ;
      }
      // Each gang will see the old k from before the loop above.
      if (k != 999)
        save.assignLoopFoundBadVal = true;
      // Each gang will see the final l from the loop above.
      if (l != 2)
        save.assignNestedLoopFoundBadVal = true;

      // PRT: k = 999;
      k = 999;
      // k should have been dropped from the loop control variable list, so it
      // should now be implicitly shared in all cases.
      //
      //           DMP: ACCLoopDirective
      //      DMP-NEXT:   ACCAutoClause
      //       DMP-NOT:     <implicit>
      // DMP-GANG-NEXT:   ACCGangClause
      //  DMP-GANG-NOT:     <implicit>
      //      DMP-NEXT:   ACCWorkerClause
      //       DMP-NOT:     <implicit>
      //      DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      //      DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
      //      DMP-NEXT:   impl: ForStmt
      //
      //           PRT-A-NEXT: {{^ *}}#pragma acc loop auto
      //       PRT-A-SRC-SAME: {{^ }}GANG
      //  PRT-A-AST-GANG-SAME: {{^ }}gang
      //           PRT-A-SAME: {{^ }}worker
      //          PRT-AO-SAME: {{^ }}// discarded in OpenMP translation
      //           PRT-A-SAME: {{^$}}
      //          PRT-OA-NEXT: {{^ *}}// #pragma acc loop auto
      //      PRT-OA-SRC-SAME: {{^ }}GANG
      // PRT-OA-AST-GANG-SAME: {{^ }}gang
      //          PRT-OA-SAME: {{^ }}worker
      //          PRT-OA-SAME: {{^ }}// discarded in OpenMP translation{{$}}
      //             PRT-NEXT: for (int i = {{.*}}) {
      //             PRT-NEXT:   {{printf|TGT_PRINTF}}
      //             PRT-NEXT:   k = 9999;
      //             PRT-NEXT: }
      //
      // Gang-redundant mode executes each loop iteration per gang.
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      #pragma acc loop auto GANG worker
      for (int i = 0; i < 2; ++i) {
        TGT_PRINTF("loop iteration\n");
        k = 9999;
      }
      // Each gang will see the k from the i loop above.
      if (k != 9999)
        save.writeInLaterLoopFoundBadVal = true;
    } // PRT: }

    // EXE-NEXT: save.loopOnlyAtI0FoundBadVal=0
    // EXE-NEXT: save.loopOnlyAtI1FoundBadVal=0
    // EXE-NEXT: save.loopOnlyArrFoundOldVal=1
    // EXE-NEXT: save.loopOnlyArrFoundBadVal=0
    // EXE-NEXT: save.declNestedLoopFoundBadVal=0
    // EXE-NEXT: save.assignLoopFoundBadVal=0
    // EXE-NEXT: save.assignNestedLoopFoundBadVal=0
    // EXE-NEXT: save.writeInLaterLoopFoundBadVal=0
    // EXE-NEXT: loopOnly=88
    // EXE-NEXT: loopOnlyArr[0]=77
    printf("save.loopOnlyAtI0FoundBadVal=%d\n", save.loopOnlyAtI0FoundBadVal);
    printf("save.loopOnlyAtI1FoundBadVal=%d\n", save.loopOnlyAtI1FoundBadVal);
    printf("save.loopOnlyArrFoundOldVal=%d\n", save.loopOnlyArrFoundOldVal);
    printf("save.loopOnlyArrFoundBadVal=%d\n", save.loopOnlyArrFoundBadVal);
    printf("save.declNestedLoopFoundBadVal=%d\n", save.declNestedLoopFoundBadVal);
    printf("save.assignLoopFoundBadVal=%d\n", save.assignLoopFoundBadVal);
    printf("save.assignNestedLoopFoundBadVal=%d\n", save.assignNestedLoopFoundBadVal);
    printf("save.writeInLaterLoopFoundBadVal=%d\n", save.writeInLaterLoopFoundBadVal);
    printf("loopOnly=%d\n", loopOnly);
    printf("loopOnlyArr[0]=%d\n", loopOnlyArr[0]);

    //. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 
    // acc loop auto GANG vector
    //. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 

    // DMP-LABEL: StringLiteral {{.*}} "acc loop auto GANG vector\n"
    // PRT-LABEL: printf("acc loop auto GANG vector\n");
    //   EXE-NOT: {{.}}
    // EXE-LABEL: acc loop auto GANG vector{{$}}
    //   EXE-NOT: {{.}}
    memset(&save, 0, sizeof(save));
    loopOnly = 88;
    loopOnlyArr[0] = 88;
    printf("acc loop auto GANG vector\n");

    //      DMP: ACCParallelDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 3
    // DMP-NEXT:   ACCNomapClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnly' 'int'
    // DMP-NEXT:   ACCCopyClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'save'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'save'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    // DMP-NEXT:   ACCFirstprivateClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnly' 'int'
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 3
    // DMP-NEXT:     OMPMapClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'save'
    // DMP-NEXT:       DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    // DMP-NEXT:     OMPSharedClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'save'
    // DMP-NEXT:       DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    // DMP-NEXT:     OMPFirstprivateClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'loopOnly' 'int'
    //      DMP:     CompoundStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc parallel num_gangs(3){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(3) map(ompx_hold,tofrom: save,loopOnlyArr) shared(save,loopOnlyArr) firstprivate(loopOnly){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(3) map(ompx_hold,tofrom: save,loopOnlyArr) shared(save,loopOnlyArr) firstprivate(loopOnly){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(3){{$}}
    //    PRT-NEXT: {
    #pragma acc parallel num_gangs(3)
    {
      // PRT-NEXT: int j = 0;
      int j = 0;
      //           DMP: ACCLoopDirective
      //      DMP-NEXT:   ACCAutoClause
      //       DMP-NOT:     <implicit>
      // DMP-GANG-NEXT:   ACCGangClause
      //  DMP-GANG-NOT:     <implicit>
      //      DMP-NEXT:   ACCVectorClause
      //       DMP-NOT:     <implicit>
      //      DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      //      DMP-NEXT:     DeclRefExpr {{.*}} 'j' 'int'
      //      DMP-NEXT:     DeclRefExpr {{.*}} 'save'
      //      DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnly' 'int'
      //      DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
      //      DMP-NEXT:   impl: ForStmt
      //
      //           PRT-A-NEXT: {{^ *}}#pragma acc loop auto
      //       PRT-A-SRC-SAME: {{^ }}GANG
      //  PRT-A-AST-GANG-SAME: {{^ }}gang
      //           PRT-A-SAME: {{^ }}vector
      //          PRT-AO-SAME: {{^ }}// discarded in OpenMP translation
      //           PRT-A-SAME: {{^$}}
      //          PRT-OA-NEXT: {{^ *}}// #pragma acc loop auto
      //      PRT-OA-SRC-SAME: {{^ }}GANG
      // PRT-OA-AST-GANG-SAME: {{^ }}gang
      //          PRT-OA-SAME: {{^ }}vector
      //          PRT-OA-SAME: {{^ }}// discarded in OpenMP translation{{$}}
      //             PRT-NEXT: for (int i = {{.*}}) {
      //             PRT-NEXT:   {{printf|TGT_PRINTF}}
      //             PRT-NEXT:   for (j ={{.*}})
      //             PRT-NEXT:     ;
      //                  PRT: }
      //
      // Gang-redundant mode executes each loop iteration per gang.
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      #pragma acc loop auto GANG vector
      for (int i = 0; i < 2; ++i) {
        TGT_PRINTF("loop iteration\n");
        for (j = 0; j < 2; ++j)
          ;
        checkLoopSeqPrivate(save, i, loopOnly, 88, 77);
        loopOnly = 77;
        checkLoopSeqShared(save, i, loopOnlyArr, 88, 77);
        loopOnlyArr[0] = 77;
      }
      // Each gang will see the final j from the j loop above.
      if (j != 2)
        save.declNestedLoopFoundBadVal = true;

      //      PRT: int k = 999;
      // PRT-NEXT: int l = 888;
      int k = 999;
      int l = 888;
      //           DMP: ACCLoopDirective
      //      DMP-NEXT:   ACCAutoClause
      //       DMP-NOT:     <implicit>
      // DMP-GANG-NEXT:   ACCGangClause
      //  DMP-GANG-NOT:     <implicit>
      //      DMP-NEXT:   ACCVectorClause
      //       DMP-NOT:     <implicit>
      //      DMP-NEXT:   ACCPrivateClause {{.*}} <predetermined>
      //      DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
      //      DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      //      DMP-NEXT:     DeclRefExpr {{.*}} 'l' 'int'
      //      DMP-NEXT:   impl: CompoundStmt
      //      DMP-NEXT:     DeclStmt
      //      DMP-NEXT:       VarDecl {{.*}} k 'int'
      //      DMP-NEXT:     ForStmt
      //
      //          PRT-AO-NEXT: // v----------ACC----------v
      //           PRT-A-NEXT: {{^ *}}#pragma acc loop auto
      //       PRT-A-SRC-SAME: {{^ }}GANG
      //  PRT-A-AST-GANG-SAME: {{^ }}gang
      //           PRT-A-SAME: {{^ }}vector{{$}}
      //           PRT-A-NEXT: for (k ={{.*}}) {
      //           PRT-A-NEXT:   {{print|TGT_PRINTF}}
      //           PRT-A-NEXT:   for (l ={{.*}})
      //           PRT-A-NEXT:     ;
      //           PRT-A-NEXT: }
      //          PRT-AO-NEXT: // ---------ACC->OMP--------
      //          PRT-AO-NEXT: // {
      //          PRT-AO-NEXT: //   int k;
      //          PRT-AO-NEXT: //   for (k ={{.*}}) {
      //          PRT-AO-NEXT: //     {{printf|TGT_PRINTF}}
      //          PRT-AO-NEXT: //     for (l ={{.*}})
      //          PRT-AO-NEXT: //       ;
      //          PRT-AO-NEXT: //   }
      //          PRT-AO-NEXT: // }
      //          PRT-AO-NEXT: // ^----------OMP----------^
      //
      //          PRT-OA-NEXT: // v----------OMP----------v
      //           PRT-O-NEXT: {
      //           PRT-O-NEXT:   int k;
      //           PRT-O-NEXT:   for (k ={{.*}}) {
      //           PRT-O-NEXT:     {{printf|TGT_PRINTF}}
      //           PRT-O-NEXT:     for (l ={{.*}})
      //           PRT-O-NEXT:       ;
      //           PRT-O-NEXT:   }
      //           PRT-O-NEXT: }
      //          PRT-OA-NEXT: // ---------OMP<-ACC--------
      //          PRT-OA-NEXT: {{^ *}}// #pragma acc loop auto
      //      PRT-OA-SRC-SAME: {{^ }}GANG
      // PRT-OA-AST-GANG-SAME: {{^ }}gang
      //          PRT-OA-SAME: {{^ }}vector{{$}}
      //          PRT-OA-NEXT: // for (k ={{.*}}) {
      //          PRT-OA-NEXT: //   {{printf|TGT_PRINTF}}
      //          PRT-OA-NEXT: //   for (l ={{.*}})
      //          PRT-OA-NEXT: //     ;
      //          PRT-OA-NEXT: // }
      //          PRT-OA-NEXT: // ^----------ACC----------^
      //
      // PRT-NOACC-NEXT: for (k ={{.*}}) {
      // PRT-NOACC-NEXT:   {{printf|TGT_PRINTF}}
      // PRT-NOACC-NEXT:   for (l ={{.*}})
      // PRT-NOACC-NEXT:     ;
      // PRT-NOACC-NEXT: }
      //
      // Gang-redundant mode executes each loop iteration per gang.
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      #pragma acc loop auto GANG vector
      for (k = 1; k < 3; ++k) {
        TGT_PRINTF("loop iteration\n");
        for (l = 0; l < 2; ++l)
          ;
      }
      // Each gang will see the old k from before the loop above.
      if (k != 999)
        save.assignLoopFoundBadVal = true;
      // Each gang will see the final l from the loop above.
      if (l != 2)
        save.assignNestedLoopFoundBadVal = true;

      // PRT: k = 999;
      k = 999;
      // k should have been dropped from the loop control variable list, so it
      // should now be implicitly shared in all cases.
      //
      //           DMP: ACCLoopDirective
      //      DMP-NEXT:   ACCAutoClause
      //       DMP-NOT:     <implicit>
      // DMP-GANG-NEXT:   ACCGangClause
      //  DMP-GANG-NOT:     <implicit>
      //      DMP-NEXT:   ACCVectorClause
      //       DMP-NOT:     <implicit>
      //      DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      //      DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
      //      DMP-NEXT:   impl: ForStmt
      //
      //           PRT-A-NEXT: {{^ *}}#pragma acc loop auto
      //       PRT-A-SRC-SAME: {{^ }}GANG
      //  PRT-A-AST-GANG-SAME: {{^ }}gang
      //           PRT-A-SAME: {{^ }}vector
      //          PRT-AO-SAME: {{^ }}// discarded in OpenMP translation
      //           PRT-A-SAME: {{^$}}
      //          PRT-OA-NEXT: {{^ *}}// #pragma acc loop auto
      //      PRT-OA-SRC-SAME: {{^ }}GANG
      // PRT-OA-AST-GANG-SAME: {{^ }}gang
      //          PRT-OA-SAME: {{^ }}vector
      //          PRT-OA-SAME: {{^ }}// discarded in OpenMP translation{{$}}
      //             PRT-NEXT: for (int i = {{.*}}) {
      //             PRT-NEXT:   {{printf|TGT_PRINTF}}
      //             PRT-NEXT:   k = 9999;
      //                  PRT: }
      //
      // Gang-redundant mode executes each loop iteration per gang.
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      #pragma acc loop auto GANG vector
      for (int i = 0; i < 2; ++i) {
        TGT_PRINTF("loop iteration\n");
        k = 9999;
      }
      // Each gang will see the k from the i loop above.
      if (k != 9999)
        save.writeInLaterLoopFoundBadVal = true;
    }

    // EXE-NEXT: save.loopOnlyAtI0FoundBadVal=0
    // EXE-NEXT: save.loopOnlyAtI1FoundBadVal=0
    // EXE-NEXT: save.loopOnlyArrFoundOldVal=1
    // EXE-NEXT: save.loopOnlyArrFoundBadVal=0
    // EXE-NEXT: save.declNestedLoopFoundBadVal=0
    // EXE-NEXT: save.assignLoopFoundBadVal=0
    // EXE-NEXT: save.assignNestedLoopFoundBadVal=0
    // EXE-NEXT: save.writeInLaterLoopFoundBadVal=0
    // EXE-NEXT: loopOnly=88
    // EXE-NEXT: loopOnlyArr[0]=77
    printf("save.loopOnlyAtI0FoundBadVal=%d\n", save.loopOnlyAtI0FoundBadVal);
    printf("save.loopOnlyAtI1FoundBadVal=%d\n", save.loopOnlyAtI1FoundBadVal);
    printf("save.loopOnlyArrFoundOldVal=%d\n", save.loopOnlyArrFoundOldVal);
    printf("save.loopOnlyArrFoundBadVal=%d\n", save.loopOnlyArrFoundBadVal);
    printf("save.declNestedLoopFoundBadVal=%d\n", save.declNestedLoopFoundBadVal);
    printf("save.assignLoopFoundBadVal=%d\n", save.assignLoopFoundBadVal);
    printf("save.assignNestedLoopFoundBadVal=%d\n", save.assignNestedLoopFoundBadVal);
    printf("save.writeInLaterLoopFoundBadVal=%d\n", save.writeInLaterLoopFoundBadVal);
    printf("loopOnly=%d\n", loopOnly);
    printf("loopOnlyArr[0]=%d\n", loopOnlyArr[0]);

    //. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 
    // acc loop auto GANG worker vector
    //. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 

    // DMP-LABEL: StringLiteral {{.*}} "acc loop auto GANG worker vector\n"
    // PRT-LABEL: printf("acc loop auto GANG worker vector\n");
    //   EXE-NOT: {{.}}
    // EXE-LABEL: acc loop auto GANG worker vector{{$}}
    //   EXE-NOT: {{.}}
    memset(&save, 0, sizeof(save));
    loopOnly = 88;
    loopOnlyArr[0] = 88;
    printf("acc loop auto GANG worker vector\n");

    //      DMP: ACCParallelDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 3
    // DMP-NEXT:   ACCNomapClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnly' 'int'
    // DMP-NEXT:   ACCCopyClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'save'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'save'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    // DMP-NEXT:   ACCFirstprivateClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnly' 'int'
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 3
    // DMP-NEXT:     OMPMapClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'save'
    // DMP-NEXT:       DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    // DMP-NEXT:     OMPSharedClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'save'
    // DMP-NEXT:       DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    // DMP-NEXT:     OMPFirstprivateClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'loopOnly' 'int'
    //      DMP:     CompoundStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc parallel num_gangs(3){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(3) map(ompx_hold,tofrom: save,loopOnlyArr) shared(save,loopOnlyArr) firstprivate(loopOnly){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(3) map(ompx_hold,tofrom: save,loopOnlyArr) shared(save,loopOnlyArr) firstprivate(loopOnly){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(3){{$}}
    //    PRT-NEXT: {
    #pragma acc parallel num_gangs(3)
    {
      // PRT-NEXT: int j = 0;
      int j = 0;
      //           DMP: ACCLoopDirective
      //      DMP-NEXT:   ACCAutoClause
      //       DMP-NOT:     <implicit>
      // DMP-GANG-NEXT:   ACCGangClause
      //  DMP-GANG-NOT:     <implicit>
      //      DMP-NEXT:   ACCWorkerClause
      //       DMP-NOT:     <implicit>
      //      DMP-NEXT:   ACCVectorClause
      //       DMP-NOT:     <implicit>
      //      DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      //      DMP-NEXT:     DeclRefExpr {{.*}} 'j' 'int'
      //      DMP-NEXT:     DeclRefExpr {{.*}} 'save'
      //      DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnly' 'int'
      //      DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
      //      DMP-NEXT:   impl: ForStmt
      //
      //           PRT-A-NEXT: {{^ *}}#pragma acc loop auto
      //       PRT-A-SRC-SAME: {{^ }}GANG
      //  PRT-A-AST-GANG-SAME: {{^ }}gang
      //           PRT-A-SAME: {{^ }}worker vector
      //          PRT-AO-SAME: {{^ }}// discarded in OpenMP translation
      //           PRT-A-SAME: {{^$}}
      //          PRT-OA-NEXT: {{^ *}}// #pragma acc loop auto
      //      PRT-OA-SRC-SAME: {{^ }}GANG
      // PRT-OA-AST-GANG-SAME: {{^ }}gang
      //          PRT-OA-SAME: {{^ }}worker vector
      //          PRT-OA-SAME: {{^ }}// discarded in OpenMP translation{{$}}
      //             PRT-NEXT: for (int i = {{.*}}) {
      //             PRT-NEXT:   {{print|TGT_PRINTF}}
      //             PRT-NEXT:   for (j ={{.*}})
      //             PRT-NEXT:     ;
      //                  PRT: }
      //
      // Gang-redundant mode executes each loop iteration per gang.
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      #pragma acc loop auto GANG worker vector
      for (int i = 0; i < 2; ++i) {
        TGT_PRINTF("loop iteration\n");
        for (j = 0; j < 2; ++j)
          ;
        checkLoopSeqPrivate(save, i, loopOnly, 88, 77);
        loopOnly = 77;
        checkLoopSeqShared(save, i, loopOnlyArr, 88, 77);
        loopOnlyArr[0] = 77;
      }
      // Each gang will see the final j from the j loop above.
      if (j != 2)
        save.declNestedLoopFoundBadVal = true;

      //      PRT: int k = 999;
      // PRT-NEXT: int l = 888;
      int k = 999;
      int l = 888;
      //           DMP: ACCLoopDirective
      //      DMP-NEXT:   ACCAutoClause
      //       DMP-NOT:     <implicit>
      // DMP-GANG-NEXT:   ACCGangClause
      //  DMP-GANG-NOT:     <implicit>
      //      DMP-NEXT:   ACCWorkerClause
      //       DMP-NOT:     <implicit>
      //      DMP-NEXT:   ACCVectorClause
      //       DMP-NOT:     <implicit>
      //      DMP-NEXT:   ACCPrivateClause {{.*}} <predetermined>
      //      DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
      //      DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      //      DMP-NEXT:     DeclRefExpr {{.*}} 'l' 'int'
      //      DMP-NEXT:   impl: CompoundStmt
      //      DMP-NEXT:     DeclStmt
      //      DMP-NEXT:       VarDecl {{.*}} k 'int'
      //      DMP-NEXT:     ForStmt
      //
      //          PRT-AO-NEXT: // v----------ACC----------v
      //           PRT-A-NEXT: {{^ *}}#pragma acc loop auto
      //       PRT-A-SRC-SAME: {{^ }}GANG
      //  PRT-A-AST-GANG-SAME: {{^ }}gang
      //           PRT-A-SAME: {{^ }}worker vector{{$}}
      //           PRT-A-NEXT: for (k ={{.*}}) {
      //           PRT-A-NEXT:   {{print|TGT_PRINTF}}
      //           PRT-A-NEXT:   for (l ={{.*}})
      //           PRT-A-NEXT:     ;
      //           PRT-A-NEXT: }
      //          PRT-AO-NEXT: // ---------ACC->OMP--------
      //          PRT-AO-NEXT: // {
      //          PRT-AO-NEXT: //   int k;
      //          PRT-AO-NEXT: //   for (k ={{.*}}) {
      //          PRT-AO-NEXT: //     {{printf|TGT_PRINTF}}
      //          PRT-AO-NEXT: //     for (l ={{.*}})
      //          PRT-AO-NEXT: //       ;
      //          PRT-AO-NEXT: //   }
      //          PRT-AO-NEXT: // }
      //          PRT-AO-NEXT: // ^----------OMP----------^
      //
      //          PRT-OA-NEXT: // v----------OMP----------v
      //           PRT-O-NEXT: {
      //           PRT-O-NEXT:   int k;
      //           PRT-O-NEXT:   for (k ={{.*}}) {
      //           PRT-O-NEXT:     {{printf|TGT_PRINTF}}
      //           PRT-O-NEXT:     for (l ={{.*}})
      //           PRT-O-NEXT:       ;
      //           PRT-O-NEXT:   }
      //           PRT-O-NEXT: }
      //          PRT-OA-NEXT: // ---------OMP<-ACC--------
      //          PRT-OA-NEXT: {{^ *}}// #pragma acc loop auto
      //      PRT-OA-SRC-SAME: {{^ }}GANG
      // PRT-OA-AST-GANG-SAME: {{^ }}gang
      //          PRT-OA-SAME: {{^ }}worker vector{{$}}
      //          PRT-OA-NEXT: // for (k ={{.*}}) {
      //          PRT-OA-NEXT: //   {{printf|TGT_PRINTF}}
      //          PRT-OA-NEXT: //   for (l ={{.*}})
      //          PRT-OA-NEXT: //     ;
      //          PRT-OA-NEXT: // }
      //          PRT-OA-NEXT: // ^----------ACC----------^
      //
      // PRT-NOACC-NEXT: for (k ={{.*}}) {
      // PRT-NOACC-NEXT:   {{printf|TGT_PRINTF}}
      // PRT-NOACC-NEXT:   for (l ={{.*}})
      // PRT-NOACC-NEXT:     ;
      // PRT-NOACC-NEXT: }
      //
      // Gang-redundant mode executes each loop iteration per gang.
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      #pragma acc loop auto GANG worker vector
      for (k = 1; k < 3; ++k) {
        TGT_PRINTF("loop iteration\n");
        for (l = 0; l < 2; ++l)
          ;
      }
      // Each gang will see the old k from before the loop above.
      if (k != 999)
        save.assignLoopFoundBadVal = true;
      // Each gang will see the final l from the loop above.
      if (l != 2)
        save.assignNestedLoopFoundBadVal = true;

      // PRT: k = 999;
      k = 999;
      // k should have been dropped from the loop control variable list, so it
      // should now be implicitly shared in all cases.
      //
      //           DMP: ACCLoopDirective
      //      DMP-NEXT:   ACCAutoClause
      //       DMP-NOT:     <implicit>
      // DMP-GANG-NEXT:   ACCGangClause
      //  DMP-GANG-NOT:     <implicit>
      //      DMP-NEXT:   ACCWorkerClause
      //       DMP-NOT:     <implicit>
      //      DMP-NEXT:   ACCVectorClause
      //       DMP-NOT:     <implicit>
      //      DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      //      DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
      //      DMP-NEXT:   impl: ForStmt
      //
      //           PRT-A-NEXT: {{^ *}}#pragma acc loop auto
      //       PRT-A-SRC-SAME: {{^ }}GANG
      //  PRT-A-AST-GANG-SAME: {{^ }}gang
      //           PRT-A-SAME: {{^ }}worker vector
      //          PRT-AO-SAME: {{^ }}// discarded in OpenMP translation
      //           PRT-A-SAME: {{^$}}
      //          PRT-OA-NEXT: {{^ *}}// #pragma acc loop auto
      //      PRT-OA-SRC-SAME: {{^ }}GANG
      // PRT-OA-AST-GANG-SAME: {{^ }}gang
      //          PRT-OA-SAME: {{^ }}worker vector
      //          PRT-OA-SAME: {{^ }}// discarded in OpenMP translation{{$}}
      //             PRT-NEXT: for ({{.*}}) {
      //             PRT-NEXT:   {{print|TGT_PRINTF}}
      //             PRT-NEXT:   k = 9999;
      //             PRT-NEXT: }
      //
      // Gang-redundant mode executes each loop iteration per gang.
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      #pragma acc loop auto GANG worker vector
      for (int i = 0; i < 2; ++i) {
        TGT_PRINTF("loop iteration\n");
        k = 9999;
      }
      // Each gang will see the k from the i loop above.
      if (k != 9999)
        save.writeInLaterLoopFoundBadVal = true;
    } // PRT: }

    // EXE-NEXT: save.loopOnlyAtI0FoundBadVal=0
    // EXE-NEXT: save.loopOnlyAtI1FoundBadVal=0
    // EXE-NEXT: save.loopOnlyArrFoundOldVal=1
    // EXE-NEXT: save.loopOnlyArrFoundBadVal=0
    // EXE-NEXT: save.declNestedLoopFoundBadVal=0
    // EXE-NEXT: save.assignLoopFoundBadVal=0
    // EXE-NEXT: save.assignNestedLoopFoundBadVal=0
    // EXE-NEXT: save.writeInLaterLoopFoundBadVal=0
    // EXE-NEXT: loopOnly=88
    // EXE-NEXT: loopOnlyArr[0]=77
    printf("save.loopOnlyAtI0FoundBadVal=%d\n", save.loopOnlyAtI0FoundBadVal);
    printf("save.loopOnlyAtI1FoundBadVal=%d\n", save.loopOnlyAtI1FoundBadVal);
    printf("save.loopOnlyArrFoundOldVal=%d\n", save.loopOnlyArrFoundOldVal);
    printf("save.loopOnlyArrFoundBadVal=%d\n", save.loopOnlyArrFoundBadVal);
    printf("save.declNestedLoopFoundBadVal=%d\n", save.declNestedLoopFoundBadVal);
    printf("save.assignLoopFoundBadVal=%d\n", save.assignLoopFoundBadVal);
    printf("save.assignNestedLoopFoundBadVal=%d\n", save.assignNestedLoopFoundBadVal);
    printf("save.writeInLaterLoopFoundBadVal=%d\n", save.writeInLaterLoopFoundBadVal);
    printf("loopOnly=%d\n", loopOnly);
    printf("loopOnlyArr[0]=%d\n", loopOnlyArr[0]);
  }

  //............................................................................
  // acc loop INDEPENDENT GANG
  //............................................................................

  {
    int loopOnly;
    int loopOnlyArr[1];
    struct {
      bool loopOnlyAtI0FoundBadVal;
      bool loopOnlyAtI1FoundBadVal;

      bool loopOnlyArrFoundOldVal;
      bool loopOnlyArrFoundBadVal;

      bool declNestedLoopFoundOldVal;
      bool declNestedLoopFoundNewVal;
      bool declNestedLoopFoundBadVal;

      bool assignLoopFoundBadVal;

      bool assignNestedLoopFoundOldVal;
      bool assignNestedLoopFoundNewVal;
      bool assignNestedLoopFoundBadVal;

      bool writeInLaterLoopFoundOldVal;
      bool writeInLaterLoopFoundNewVal;
      bool writeInLaterLoopFoundBadVal;
    } save;

    // DMP-LABEL: StringLiteral {{.*}} "acc loop INDEPENDENT GANG\n"
    // PRT-LABEL: printf("acc loop INDEPENDENT GANG\n");
    //   EXE-NOT: {{.}}
    // EXE-LABEL: acc loop INDEPENDENT GANG{{$}}
    //   EXE-NOT: {{.}}
    memset(&save, 0, sizeof(save));
    loopOnly = 88;
    loopOnlyArr[0] = 88;
    printf("acc loop INDEPENDENT GANG\n");

    //      DMP: ACCParallelDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 3
    // DMP-NEXT:   ACCNomapClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnly' 'int'
    // DMP-NEXT:   ACCCopyClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'save'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'save'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    // DMP-NEXT:   ACCFirstprivateClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnly' 'int'
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 3
    // DMP-NEXT:     OMPMapClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'save'
    // DMP-NEXT:       DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    // DMP-NEXT:     OMPSharedClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'save'
    // DMP-NEXT:       DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    // DMP-NEXT:     OMPFirstprivateClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'loopOnly' 'int'
    //      DMP:     CompoundStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc parallel num_gangs(3){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(3) map(ompx_hold,tofrom: save,loopOnlyArr) shared(save,loopOnlyArr) firstprivate(loopOnly){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(3) map(ompx_hold,tofrom: save,loopOnlyArr) shared(save,loopOnlyArr) firstprivate(loopOnly){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(3){{$}}
    //    PRT-NEXT: {
    #pragma acc parallel num_gangs(3)
    {
      // PRT-NEXT: int j = 0;
      int j = 0;
      //              DMP: ACCLoopDirective
      //     DMP-IND-NEXT:   ACCIndependentClause
      //      DMP-IND-NOT:     <implicit>
      //    DMP-GANG-NEXT:   ACCGangClause
      //     DMP-GANG-NOT:     <implicit>
      //  DMP-NO-IND-NEXT:   ACCIndependentClause {{.*}} <implicit>
      //         DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      //         DMP-NEXT:     DeclRefExpr {{.*}} 'j' 'int'
      //         DMP-NEXT:     DeclRefExpr {{.*}} 'save'
      //         DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnly' 'int'
      //         DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
      // DMP-NO-GANG-NEXT:   ACCGangClause {{.*}} <implicit>
      //         DMP-NEXT:   impl: OMPDistributeDirective
      //         DMP-NEXT:     OMPSharedClause {{.*}} <implicit>
      //         DMP-NEXT:       DeclRefExpr {{.*}} 'j' 'int'
      //         DMP-NEXT:       DeclRefExpr {{.*}} 'save'
      //         DMP-NEXT:       DeclRefExpr {{.*}} 'loopOnly' 'int'
      //         DMP-NEXT:       DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
      //              DMP:     ForStmt
      //
      //           PRT-A-NEXT: {{^ *}}#pragma acc loop
      //       PRT-A-SRC-SAME: {{^ }}INDEPENDENT GANG
      //   PRT-A-AST-IND-SAME: {{^ }}independent
      //  PRT-A-AST-GANG-SAME: {{^ }}gang
      //           PRT-A-SAME: {{^$}}
      //          PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
      //           PRT-O-NEXT: {{^ *}}#pragma omp distribute{{$}}
      //          PRT-OA-NEXT: {{^ *}}// #pragma acc loop
      //      PRT-OA-SRC-SAME: {{^ }}INDEPENDENT GANG
      //  PRT-OA-AST-IND-SAME: {{^ }}independent
      // PRT-OA-AST-GANG-SAME: {{^ }}gang
      //          PRT-OA-SAME: {{^$}}
      //             PRT-NEXT: for (int i = {{.*}}) {
      //             PRT-NEXT:   {{printf|TGT_PRINTF}}
      //             PRT-NEXT:   for (j ={{.*}})
      //             PRT-NEXT:     ;
      //                  PRT: }
      //
      // Gang-partitioned mode executes each loop iteration once.
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      #pragma acc loop INDEPENDENT GANG
      for (int i = 0; i < 2; ++i) {
        TGT_PRINTF("loop iteration\n");
        for (j = 0; j < 2; ++j)
          ;
        checkLoopPartPrivate(save, i, loopOnly, 88, 77);
        loopOnly = 77;
        checkLoopPartShared(save, i, loopOnlyArr, loopOnlyArr[0], 88, 77);
        loopOnlyArr[0] = 77;
      }
      // Each gang that is assigned one or more i iterations will see the final
      // j from the j loop above, and the remaining gangs will see the old j
      // value.
      if (j == 0)
        save.declNestedLoopFoundOldVal = true;
      else if (j == 2)
        save.declNestedLoopFoundNewVal = true;
      else
        save.declNestedLoopFoundBadVal = true;

      //      PRT: int k = 999;
      // PRT-NEXT: int l = 888;
      int k = 999;
      int l = 888;
      //              DMP: ACCLoopDirective
      //     DMP-IND-NEXT:   ACCIndependentClause
      //      DMP-IND-NOT:     <implicit>
      //    DMP-GANG-NEXT:   ACCGangClause
      //     DMP-GANG-NOT:     <implicit>
      //  DMP-NO-IND-NEXT:   ACCIndependentClause {{.*}} <implicit>
      //         DMP-NEXT:   ACCPrivateClause {{.*}} <predetermined>
      //         DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
      //         DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      //         DMP-NEXT:     DeclRefExpr {{.*}} 'l' 'int'
      // DMP-NO-GANG-NEXT:   ACCGangClause {{.*}} <implicit>
      //         DMP-NEXT:   impl: OMPDistributeDirective
      //         DMP-NEXT:     OMPPrivateClause
      //          DMP-NOT:       <implicit>
      //         DMP-NEXT:       DeclRefExpr {{.*}} 'k' 'int'
      //         DMP-NEXT:     OMPSharedClause {{.*}} <implicit>
      //         DMP-NEXT:       DeclRefExpr {{.*}} 'l' 'int'
      //              DMP:     ForStmt
      //
      //           PRT-A-NEXT: {{^ *}}#pragma acc loop
      //       PRT-A-SRC-SAME: {{^ }}INDEPENDENT GANG
      //   PRT-A-AST-IND-SAME: {{^ }}independent
      //  PRT-A-AST-GANG-SAME: {{^ }}gang
      //           PRT-A-SAME: {{^$}}
      //          PRT-AO-NEXT: {{^ *}}// #pragma omp distribute private(k){{$}}
      //           PRT-O-NEXT: {{^ *}}#pragma omp distribute private(k){{$}}
      //          PRT-OA-NEXT: {{^ *}}// #pragma acc loop
      //      PRT-OA-SRC-SAME: {{^ }}INDEPENDENT GANG
      //  PRT-OA-AST-IND-SAME: {{^ }}independent
      // PRT-OA-AST-GANG-SAME: {{^ }}gang
      //          PRT-OA-SAME: {{^$}}
      //             PRT-NEXT: for (k ={{.*}}) {
      //             PRT-NEXT:   {{print|TGT_PRINTF}}
      //             PRT-NEXT:   for (l ={{.*}})
      //             PRT-NEXT:     ;
      //             PRT-NEXT: }
      //
      // Gang-partitioned mode executes each loop iteration once.
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      #pragma acc loop INDEPENDENT GANG
      for (k = 1; k < 3; ++k) {
        TGT_PRINTF("loop iteration\n");
        for (l = 0; l < 2; ++l)
          ;
      }
      // Each gang will see the old k from before the loop above.
      if (k != 999)
        save.assignLoopFoundBadVal = true;
      // Each gang that is assigned one or more k iterations will see the final
      // l from the l loop above, and the remaining gangs will see the old l
      // value.
      if (l == 888)
        save.assignNestedLoopFoundOldVal = true;
      else if (l == 2)
        save.assignNestedLoopFoundNewVal = true;
      else
        save.assignNestedLoopFoundBadVal = true;

      // PRT: k = 999;
      k = 999;
      // k should have been dropped from the loop control variable list, so it
      // should now be implicitly shared in all cases.
      //
      //              DMP: ACCLoopDirective
      //     DMP-IND-NEXT:   ACCIndependentClause
      //      DMP-IND-NOT:     <implicit>
      //    DMP-GANG-NEXT:   ACCGangClause
      //     DMP-GANG-NOT:     <implicit>
      //  DMP-NO-IND-NEXT:   ACCIndependentClause {{.*}} <implicit>
      //         DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      //         DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
      // DMP-NO-GANG-NEXT:   ACCGangClause {{.*}} <implicit>
      //         DMP-NEXT:   impl: OMPDistributeDirective
      //         DMP-NEXT:     OMPSharedClause {{.*}} <implicit>
      //         DMP-NEXT:       DeclRefExpr {{.*}} 'k' 'int'
      //              DMP:     ForStmt
      //
      //           PRT-A-NEXT: {{^ *}}#pragma acc loop
      //       PRT-A-SRC-SAME: {{^ }}INDEPENDENT GANG
      //   PRT-A-AST-IND-SAME: {{^ }}independent
      //  PRT-A-AST-GANG-SAME: {{^ }}gang
      //           PRT-A-SAME: {{^$}}
      //          PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
      //           PRT-O-NEXT: {{^ *}}#pragma omp distribute{{$}}
      //          PRT-OA-NEXT: {{^ *}}// #pragma acc loop
      //      PRT-OA-SRC-SAME: {{^ }}INDEPENDENT GANG
      //  PRT-OA-AST-IND-SAME: {{^ }}independent
      // PRT-OA-AST-GANG-SAME: {{^ }}gang
      //          PRT-OA-SAME: {{^$}}
      //             PRT-NEXT: for ({{.*}}) {
      //             PRT-NEXT:   {{print|TGT_PRINTF}}
      //             PRT-NEXT:   k = 9999;
      //             PRT-NEXT: }
      //
      // Gang-partitioned mode executes each loop iteration once.
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      #pragma acc loop INDEPENDENT GANG
      for (int i = 0; i < 2; ++i) {
        TGT_PRINTF("loop iteration\n");
        k = 9999;
      }
      // Each gang that is assigned one or more i iterations will see the k from
      // the i loop above, and the remaining gangs will see the old k value.
      if (k == 999)
        save.writeInLaterLoopFoundOldVal = true;
      else if (k == 9999)
        save.writeInLaterLoopFoundNewVal = true;
      else
        save.writeInLaterLoopFoundBadVal = true;
    } // PRT: }

    // EXE-NEXT: save.loopOnlyAtI0FoundBadVal=0
    // EXE-NEXT: save.loopOnlyAtI1FoundBadVal=0
    // EXE-NEXT: save.loopOnlyArrFoundOldVal=1
    // EXE-NEXT: save.loopOnlyArrFoundBadVal=0
    // EXE-NEXT: save.declNestedLoopFoundOldVal=1
    // EXE-NEXT: save.declNestedLoopFoundNewVal=1
    // EXE-NEXT: save.declNestedLoopFoundBadVal=0
    // EXE-NEXT: save.assignLoopFoundBadVal=0
    // EXE-NEXT: save.assignNestedLoopFoundOldVal=1
    // EXE-NEXT: save.assignNestedLoopFoundNewVal=1
    // EXE-NEXT: save.assignNestedLoopFoundBadVal=0
    // EXE-NEXT: save.writeInLaterLoopFoundOldVal=1
    // EXE-NEXT: save.writeInLaterLoopFoundNewVal=1
    // EXE-NEXT: save.writeInLaterLoopFoundBadVal=0
    // EXE-NEXT: loopOnly=88
    // EXE-NEXT: loopOnlyArr[0]=77
    printf("save.loopOnlyAtI0FoundBadVal=%d\n", save.loopOnlyAtI0FoundBadVal);
    printf("save.loopOnlyAtI1FoundBadVal=%d\n", save.loopOnlyAtI1FoundBadVal);
    printf("save.loopOnlyArrFoundOldVal=%d\n", save.loopOnlyArrFoundOldVal);
    printf("save.loopOnlyArrFoundBadVal=%d\n", save.loopOnlyArrFoundBadVal);
    printf("save.declNestedLoopFoundOldVal=%d\n", save.declNestedLoopFoundOldVal);
    printf("save.declNestedLoopFoundNewVal=%d\n", save.declNestedLoopFoundNewVal);
    printf("save.declNestedLoopFoundBadVal=%d\n", save.declNestedLoopFoundBadVal);
    printf("save.assignLoopFoundBadVal=%d\n", save.assignLoopFoundBadVal);
    printf("save.assignNestedLoopFoundOldVal=%d\n", save.assignNestedLoopFoundOldVal);
    printf("save.assignNestedLoopFoundNewVal=%d\n", save.assignNestedLoopFoundNewVal);
    printf("save.assignNestedLoopFoundBadVal=%d\n", save.assignNestedLoopFoundBadVal);
    printf("save.writeInLaterLoopFoundOldVal=%d\n", save.writeInLaterLoopFoundOldVal);
    printf("save.writeInLaterLoopFoundNewVal=%d\n", save.writeInLaterLoopFoundNewVal);
    printf("save.writeInLaterLoopFoundBadVal=%d\n", save.writeInLaterLoopFoundBadVal);
    printf("loopOnly=%d\n", loopOnly);
    printf("loopOnlyArr[0]=%d\n", loopOnlyArr[0]);
  }

  //............................................................................
  // acc loop INDEPENDENT GANG worker
  //............................................................................

  {
    int loopOnly;
    int loopOnlyArr[1];
    struct {
      bool loopOnlyFoundOldVal;
      bool loopOnlyFoundBadVal;

      bool loopOnlyArrFoundOldVal;
      bool loopOnlyArrFoundBadVal;

      bool assignLoopFoundBadVal;

      bool writeInLaterLoopFoundOldVal;
      bool writeInLaterLoopFoundNewVal;
      bool writeInLaterLoopFoundBadVal;
    } save;

    // DMP-LABEL: StringLiteral {{.*}} "acc loop INDEPENDENT GANG worker\n"
    // PRT-LABEL: printf("acc loop INDEPENDENT GANG worker\n");
    //   EXE-NOT: {{.}}
    // EXE-LABEL: acc loop INDEPENDENT GANG worker{{$}}
    //   EXE-NOT: {{.}}
    memset(&save, 0, sizeof(save));
    loopOnly = 88;
    loopOnlyArr[0] = 88;
    printf("acc loop INDEPENDENT GANG worker\n");

// FIXME: amdgcn misbehaves sometimes for worker loops.
// PRT-SRC-NEXT: #if !TGT_AMDGCN
#if !TGT_AMDGCN
    //      DMP: ACCParallelDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 3
    // DMP-NEXT:   ACCNomapClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnly' 'int'
    // DMP-NEXT:   ACCCopyClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'save'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'save'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    // DMP-NEXT:   ACCFirstprivateClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnly' 'int'
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 3
    // DMP-NEXT:     OMPMapClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'save'
    // DMP-NEXT:       DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    // DMP-NEXT:     OMPSharedClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'save'
    // DMP-NEXT:       DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    // DMP-NEXT:     OMPFirstprivateClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'loopOnly' 'int'
    //      DMP:     CompoundStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc parallel num_gangs(3){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(3) map(ompx_hold,tofrom: save,loopOnlyArr) shared(save,loopOnlyArr) firstprivate(loopOnly){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(3) map(ompx_hold,tofrom: save,loopOnlyArr) shared(save,loopOnlyArr) firstprivate(loopOnly){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(3){{$}}
    //    PRT-NEXT: {
    #pragma acc parallel num_gangs(3)
    {
      //              DMP: ACCLoopDirective
      //     DMP-IND-NEXT:   ACCIndependentClause
      //      DMP-IND-NOT:     <implicit>
      //    DMP-GANG-NEXT:   ACCGangClause
      //     DMP-GANG-NOT:     <implicit>
      //         DMP-NEXT:   ACCWorkerClause
      //          DMP-NOT:     <implicit>
      //  DMP-NO-IND-NEXT:   ACCIndependentClause {{.*}} <implicit>
      //         DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      //         DMP-NEXT:     DeclRefExpr {{.*}} 'save'
      //         DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnly' 'int'
      //         DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
      // DMP-NO-GANG-NEXT:   ACCGangClause {{.*}} <implicit>
      //         DMP-NEXT:   impl: OMPDistributeParallelForDirective
      //         DMP-NEXT:     OMPSharedClause
      //          DMP-NOT:       <implicit>
      //         DMP-NEXT:       DeclRefExpr {{.*}} 'save'
      //         DMP-NEXT:       DeclRefExpr {{.*}} 'loopOnly' 'int'
      //         DMP-NEXT:       DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
      //              DMP:     ForStmt
      //
      //           PRT-A-NEXT: {{^ *}}#pragma acc loop
      //       PRT-A-SRC-SAME: {{^ }}INDEPENDENT GANG
      //   PRT-A-AST-IND-SAME: {{^ }}independent
      //  PRT-A-AST-GANG-SAME: {{^ }}gang
      //           PRT-A-SAME: {{^ }}worker{{$}}
      //          PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for shared(save,loopOnly,loopOnlyArr){{$}}
      //           PRT-O-NEXT: {{^ *}}#pragma omp distribute parallel for shared(save,loopOnly,loopOnlyArr){{$}}
      //          PRT-OA-NEXT: {{^ *}}// #pragma acc loop
      //      PRT-OA-SRC-SAME: {{^ }}INDEPENDENT GANG
      //  PRT-OA-AST-IND-SAME: {{^ }}independent
      // PRT-OA-AST-GANG-SAME: {{^ }}gang
      //          PRT-OA-SAME: {{^ }}worker{{$}}
      //             PRT-NEXT: for (int i = {{.*}}) {
      //             PRT-NEXT:   {{printf|TGT_PRINTF}}
      //                  PRT: }
      //
      // Gang-partitioned mode executes each loop iteration once.
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      #pragma acc loop INDEPENDENT GANG worker
      for (int i = 0; i < 2; ++i) {
        TGT_PRINTF("loop iteration\n");
        // We omit checking of an inner j loop this time.  j would be shared
        // among workers and thus would have an unpredictable value.
        checkLoopPartShared(save, i, loopOnly, loopOnly, 88, 77);
        loopOnly = 77;
        checkLoopPartShared(save, i, loopOnlyArr, loopOnlyArr[0], 88, 77);
        loopOnlyArr[0] = 77;
      }

      // PRT: int k = 999;
      int k = 999;
      //              DMP: ACCLoopDirective
      //     DMP-IND-NEXT:   ACCIndependentClause
      //      DMP-IND-NOT:     <implicit>
      //    DMP-GANG-NEXT:   ACCGangClause
      //     DMP-GANG-NOT:     <implicit>
      //         DMP-NEXT:   ACCWorkerClause
      //          DMP-NOT:     <implicit>
      //  DMP-NO-IND-NEXT:   ACCIndependentClause {{.*}} <implicit>
      //         DMP-NEXT:   ACCPrivateClause {{.*}} <predetermined>
      //         DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
      // DMP-NO-GANG-NEXT:   ACCGangClause {{.*}} <implicit>
      //         DMP-NEXT:   impl: OMPDistributeParallelForDirective
      //         DMP-NEXT:     OMPPrivateClause
      //          DMP-NOT:       <implicit>
      //         DMP-NEXT:       DeclRefExpr {{.*}} 'k' 'int'
      //              DMP:     ForStmt
      //
      //           PRT-A-NEXT: {{^ *}}#pragma acc loop
      //       PRT-A-SRC-SAME: {{^ }}INDEPENDENT GANG
      //   PRT-A-AST-IND-SAME: {{^ }}independent
      //  PRT-A-AST-GANG-SAME: {{^ }}gang
      //           PRT-A-SAME: {{^ }}worker{{$}}
      //          PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for private(k){{$}}
      //           PRT-O-NEXT: {{^ *}}#pragma omp distribute parallel for private(k){{$}}
      //          PRT-OA-NEXT: {{^ *}}// #pragma acc loop
      //      PRT-OA-SRC-SAME: {{^ }}INDEPENDENT GANG
      //  PRT-OA-AST-IND-SAME: {{^ }}independent
      // PRT-OA-AST-GANG-SAME: {{^ }}gang
      //          PRT-OA-SAME: {{^ }}worker{{$}}
      //             PRT-NEXT: for (k ={{.*}}) {
      //             PRT-NEXT:   {{printf|TGT_PRINTF}}
      //             PRT-NEXT: }
      //
      // Gang-partitioned mode executes each loop iteration once.
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      #pragma acc loop INDEPENDENT GANG worker
      for (k = 1; k < 3; ++k) {
        TGT_PRINTF("loop iteration\n");
        // We omit checking of an inner l loop this time.  l would be shared
        // among workers and thus would have an unpredictable value.
      }
      // Each gang will see the old k from before the loop above.
      if (k != 999)
        save.assignLoopFoundBadVal = true;

      // PRT: k = 999;
      k = 999;
      // k should have been dropped from the loop control variable list, so it
      // should now be implicitly shared in all cases.
      //
      //              DMP: ACCLoopDirective
      //     DMP-IND-NEXT:   ACCIndependentClause
      //      DMP-IND-NOT:     <implicit>
      //    DMP-GANG-NEXT:   ACCGangClause
      //     DMP-GANG-NOT:     <implicit>
      //         DMP-NEXT:   ACCWorkerClause
      //          DMP-NOT:     <implicit>
      //  DMP-NO-IND-NEXT:   ACCIndependentClause {{.*}} <implicit>
      //         DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      //         DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
      // DMP-NO-GANG-NEXT:   ACCGangClause {{.*}} <implicit>
      //         DMP-NEXT:   impl: OMPDistributeParallelForDirective
      //         DMP-NEXT:     OMPSharedClause
      //          DMP-NOT:       <implicit>
      //         DMP-NEXT:       DeclRefExpr {{.*}} 'k' 'int'
      //              DMP:     ForStmt
      //
      //           PRT-A-NEXT: {{^ *}}#pragma acc loop
      //       PRT-A-SRC-SAME: {{^ }}INDEPENDENT GANG
      //   PRT-A-AST-IND-SAME: {{^ }}independent
      //  PRT-A-AST-GANG-SAME: {{^ }}gang
      //           PRT-A-SAME: {{^ }}worker{{$}}
      //          PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for shared(k){{$}}
      //           PRT-O-NEXT: {{^ *}}#pragma omp distribute parallel for shared(k){{$}}
      //          PRT-OA-NEXT: {{^ *}}// #pragma acc loop
      //      PRT-OA-SRC-SAME: {{^ }}INDEPENDENT GANG
      //  PRT-OA-AST-IND-SAME: {{^ }}independent
      // PRT-OA-AST-GANG-SAME: {{^ }}gang
      //          PRT-OA-SAME: {{^ }}worker{{$}}
      //             PRT-NEXT: for ({{.*}}) {
      //             PRT-NEXT:   {{printf|TGT_PRINTF}}
      //             PRT-NEXT:   k = 9999;
      //             PRT-NEXT: }
      //
      // Gang-partitioned mode executes each loop iteration once.
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      #pragma acc loop INDEPENDENT GANG worker
      for (int i = 0; i < 2; ++i) {
        TGT_PRINTF("loop iteration\n");
        k = 9999;
      }
      // Each gang that is assigned one or more i iterations will see the k from
      // the i loop above, and the remaining gangs will see the old k value.
      if (k == 999)
        save.writeInLaterLoopFoundOldVal = true;
      else if (k == 9999)
        save.writeInLaterLoopFoundNewVal = true;
      else
        save.writeInLaterLoopFoundBadVal = true;
    } // PRT: }
#endif

    // EXE-NEXT: save.loopOnlyFoundOldVal=1
    // EXE-NEXT: save.loopOnlyFoundBadVal=0
    // EXE-NEXT: save.loopOnlyArrFoundOldVal=1
    // EXE-NEXT: save.loopOnlyArrFoundBadVal=0
    // EXE-NEXT: save.assignLoopFoundBadVal=0
    // EXE-NEXT: save.writeInLaterLoopFoundOldVal=1
    // EXE-NEXT: save.writeInLaterLoopFoundNewVal=1
    // EXE-NEXT: save.writeInLaterLoopFoundBadVal=0
    // EXE-NEXT: loopOnly=88
    // EXE-NEXT: loopOnlyArr[0]=77
// FIXME: amdgcn misbehaves sometimes for worker loops.
#if !TGT_AMDGCN
    printf("save.loopOnlyFoundOldVal=%d\n", save.loopOnlyFoundOldVal);
    printf("save.loopOnlyFoundBadVal=%d\n", save.loopOnlyFoundBadVal);
    printf("save.loopOnlyArrFoundOldVal=%d\n", save.loopOnlyArrFoundOldVal);
    printf("save.loopOnlyArrFoundBadVal=%d\n", save.loopOnlyArrFoundBadVal);
    printf("save.assignLoopFoundBadVal=%d\n", save.assignLoopFoundBadVal);
    printf("save.writeInLaterLoopFoundOldVal=%d\n", save.writeInLaterLoopFoundOldVal);
    printf("save.writeInLaterLoopFoundNewVal=%d\n", save.writeInLaterLoopFoundNewVal);
    printf("save.writeInLaterLoopFoundBadVal=%d\n", save.writeInLaterLoopFoundBadVal);
    printf("loopOnly=%d\n", loopOnly);
    printf("loopOnlyArr[0]=%d\n", loopOnlyArr[0]);
#else
    printf("save.loopOnlyFoundOldVal=%d\n", 1);
    printf("save.loopOnlyFoundBadVal=%d\n", 0);
    printf("save.loopOnlyArrFoundOldVal=%d\n", 1);
    printf("save.loopOnlyArrFoundBadVal=%d\n", 0);
    printf("save.assignLoopFoundBadVal=%d\n", 0);
    printf("save.writeInLaterLoopFoundOldVal=%d\n", 1);
    printf("save.writeInLaterLoopFoundNewVal=%d\n", 1);
    printf("save.writeInLaterLoopFoundBadVal=%d\n", 0);
    printf("loopOnly=%d\n", 88);
    printf("loopOnlyArr[0]=%d\n", 77);
#endif
  }

  //............................................................................
  // acc loop INDEPENDENT GANG vector
  //............................................................................

  {
    int loopOnly;
    int loopOnlyArr[1];
    struct {
      bool loopOnlyFoundOldVal;
      bool loopOnlyFoundBadVal;

      bool loopOnlyArrFoundOldVal;
      bool loopOnlyArrFoundBadVal;

      bool assignLoopFoundBadVal;

      bool writeInLaterLoopFoundOldVal;
      bool writeInLaterLoopFoundNewVal;
      bool writeInLaterLoopFoundBadVal;
    } save;

    // DMP-LABEL: StringLiteral {{.*}} "acc loop INDEPENDENT GANG vector\n"
    // PRT-LABEL: printf("acc loop INDEPENDENT GANG vector\n");
    //   EXE-NOT: {{.}}
    // EXE-LABEL: acc loop INDEPENDENT GANG vector{{$}}
    //   EXE-NOT: {{.}}
    memset(&save, 0, sizeof(save));
    loopOnly = 88;
    loopOnlyArr[0] = 88;
    printf("acc loop INDEPENDENT GANG vector\n");

    //      DMP: ACCParallelDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 3
    // DMP-NEXT:   ACCNomapClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnly' 'int'
    // DMP-NEXT:   ACCCopyClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'save'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'save'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    // DMP-NEXT:   ACCFirstprivateClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnly' 'int'
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 3
    // DMP-NEXT:     OMPMapClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'save'
    // DMP-NEXT:       DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    // DMP-NEXT:     OMPSharedClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'save'
    // DMP-NEXT:       DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    // DMP-NEXT:     OMPFirstprivateClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'loopOnly' 'int'
    //      DMP:     CompoundStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc parallel num_gangs(3){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(3) map(ompx_hold,tofrom: save,loopOnlyArr) shared(save,loopOnlyArr) firstprivate(loopOnly){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(3) map(ompx_hold,tofrom: save,loopOnlyArr) shared(save,loopOnlyArr) firstprivate(loopOnly){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(3){{$}}
    //    PRT-NEXT: {
    #pragma acc parallel num_gangs(3)
    {
      //              DMP: ACCLoopDirective
      //     DMP-IND-NEXT:   ACCIndependentClause
      //      DMP-IND-NOT:     <implicit>
      //    DMP-GANG-NEXT:   ACCGangClause
      //     DMP-GANG-NOT:     <implicit>
      //         DMP-NEXT:   ACCVectorClause
      //          DMP-NOT:     <implicit>
      //  DMP-NO-IND-NEXT:   ACCIndependentClause {{.*}} <implicit>
      //         DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      //         DMP-NEXT:     DeclRefExpr {{.*}} 'save'
      //         DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnly' 'int'
      //         DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
      // DMP-NO-GANG-NEXT:   ACCGangClause {{.*}} <implicit>
      //         DMP-NEXT:   impl: OMPDistributeSimdDirective
      //         DMP-NEXT:     OMPSharedClause {{.*}} <implicit>
      //         DMP-NEXT:       DeclRefExpr {{.*}} 'save'
      //         DMP-NEXT:       DeclRefExpr {{.*}} 'loopOnly' 'int'
      //         DMP-NEXT:       DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
      //              DMP:     ForStmt
      //
      //           PRT-A-NEXT: {{^ *}}#pragma acc loop
      //       PRT-A-SRC-SAME: {{^ }}INDEPENDENT GANG
      //   PRT-A-AST-IND-SAME: {{^ }}independent
      //  PRT-A-AST-GANG-SAME: {{^ }}gang
      //           PRT-A-SAME: {{^ }}vector{{$}}
      //          PRT-AO-NEXT: {{^ *}}// #pragma omp distribute simd{{$}}
      //           PRT-O-NEXT: {{^ *}}#pragma omp distribute simd{{$}}
      //          PRT-OA-NEXT: {{^ *}}// #pragma acc loop
      //      PRT-OA-SRC-SAME: {{^ }}INDEPENDENT GANG
      //  PRT-OA-AST-IND-SAME: {{^ }}independent
      // PRT-OA-AST-GANG-SAME: {{^ }}gang
      //          PRT-OA-SAME: {{^ }}vector{{$}}
      //             PRT-NEXT: for (int i = {{.*}}) {
      //             PRT-NEXT:   {{printf|TGT_PRINTF}}
      //                  PRT: }
      //
      // Gang-partitioned mode executes each loop iteration once.
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      #pragma acc loop INDEPENDENT GANG vector
      for (int i = 0; i < 2; ++i) {
        TGT_PRINTF("loop iteration\n");
        // We omit checking of an inner j loop this time.  j would be shared
        // among vector lanes and thus would have an unpredictable value.
        checkLoopPartShared(save, i, loopOnly, loopOnly, 88, 77);
        loopOnly = 77;
        checkLoopPartShared(save, i, loopOnlyArr, loopOnlyArr[0], 88, 77);
        loopOnlyArr[0] = 77;
      }

      // PRT: int k = 999;
      int k = 999;
      //              DMP: ACCLoopDirective
      //     DMP-IND-NEXT:   ACCIndependentClause
      //      DMP-IND-NOT:     <implicit>
      //    DMP-GANG-NEXT:   ACCGangClause
      //     DMP-GANG-NOT:     <implicit>
      //         DMP-NEXT:   ACCVectorClause
      //          DMP-NOT:     <implicit>
      //  DMP-NO-IND-NEXT:   ACCIndependentClause {{.*}} <implicit>
      //         DMP-NEXT:   ACCPrivateClause {{.*}} <predetermined>
      //         DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
      // DMP-NO-GANG-NEXT:   ACCGangClause {{.*}} <implicit>
      //         DMP-NEXT:   impl: CompoundStmt
      //         DMP-NEXT:     DeclStmt
      //         DMP-NEXT:       VarDecl {{.*}} k 'int'
      //         DMP-NEXT:     OMPDistributeSimdDirective
      //          DMP-NOT:       OMPPrivateClause
      //              DMP:       ForStmt
      //
      //          PRT-AO-NEXT: // v----------ACC----------v
      //           PRT-A-NEXT: {{^ *}}#pragma acc loop
      //       PRT-A-SRC-SAME: {{^ }}INDEPENDENT GANG
      //   PRT-A-AST-IND-SAME: {{^ }}independent
      //  PRT-A-AST-GANG-SAME: {{^ }}gang
      //           PRT-A-SAME: {{^ }}vector{{$}}
      //           PRT-A-NEXT: for (k ={{.*}}) {
      //           PRT-A-NEXT:   {{print|TGT_PRINTF}}
      //           PRT-A-NEXT: }
      //          PRT-AO-NEXT: // ---------ACC->OMP--------
      //          PRT-AO-NEXT: // {
      //          PRT-AO-NEXT: //   int k;
      //          PRT-AO-NEXT: //   #pragma omp distribute simd{{$}}
      //          PRT-AO-NEXT: //   for (k ={{.*}}) {
      //          PRT-AO-NEXT: //     {{printf|TGT_PRINTF}}
      //          PRT-AO-NEXT: //   }
      //          PRT-AO-NEXT: // }
      //          PRT-AO-NEXT: // ^----------OMP----------^
      //
      //          PRT-OA-NEXT: // v----------OMP----------v
      //           PRT-O-NEXT: {
      //           PRT-O-NEXT:   int k;
      //           PRT-O-NEXT:   {{^ *}}#pragma omp distribute simd{{$}}
      //           PRT-O-NEXT:   for (k ={{.*}}) {
      //           PRT-O-NEXT:     {{printf|TGT_PRINTF}}
      //           PRT-O-NEXT:   }
      //           PRT-O-NEXT: }
      //          PRT-OA-NEXT: // ---------OMP<-ACC--------
      //          PRT-OA-NEXT: {{^ *}}// #pragma acc loop
      //      PRT-OA-SRC-SAME: {{^ }}INDEPENDENT GANG
      //  PRT-OA-AST-IND-SAME: {{^ }}independent
      // PRT-OA-AST-GANG-SAME: {{^ }}gang
      //          PRT-OA-SAME: {{^ }}vector{{$}}
      //          PRT-OA-NEXT: // for (k ={{.*}}) {
      //          PRT-OA-NEXT: //   {{printf|TGT_PRINTF}}
      //          PRT-OA-NEXT: // }
      //          PRT-OA-NEXT: // ^----------ACC----------^
      //
      // PRT-NOACC-NEXT: for (k ={{.*}}) {
      // PRT-NOACC-NEXT:   {{printf|TGT_PRINTF}}
      // PRT-NOACC-NEXT: }
      //
      // Gang-partitioned mode executes each loop iteration once.
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      #pragma acc loop INDEPENDENT GANG vector
      for (k = 1; k < 3; ++k) {
        TGT_PRINTF("loop iteration\n");
        // We omit checking of an inner l loop this time.  l would be shared
        // among vector lanes and thus would have an unpredictable value.
      }
      // Each gang will see the old k from before the loop above.
      if (k != 999)
        save.assignLoopFoundBadVal = true;

      // PRT: k = 999;
      k = 999;
      // k should have been dropped from the loop control variable list, so it
      // should now be implicitly shared in all cases.
      //
      //              DMP: ACCLoopDirective
      //     DMP-IND-NEXT:   ACCIndependentClause
      //      DMP-IND-NOT:     <implicit>
      //    DMP-GANG-NEXT:   ACCGangClause
      //     DMP-GANG-NOT:     <implicit>
      //         DMP-NEXT:   ACCVectorClause
      //          DMP-NOT:     <implicit>
      //  DMP-NO-IND-NEXT:   ACCIndependentClause {{.*}} <implicit>
      //         DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      //         DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
      // DMP-NO-GANG-NEXT:   ACCGangClause {{.*}} <implicit>
      //         DMP-NEXT:   impl: OMPDistributeSimdDirective
      //         DMP-NEXT:     OMPSharedClause {{.*}} <implicit>
      //         DMP-NEXT:       DeclRefExpr {{.*}} 'k' 'int'
      //              DMP:     ForStmt
      //
      //           PRT-A-NEXT: {{^ *}}#pragma acc loop
      //       PRT-A-SRC-SAME: {{^ }}INDEPENDENT GANG
      //   PRT-A-AST-IND-SAME: {{^ }}independent
      //  PRT-A-AST-GANG-SAME: {{^ }}gang
      //           PRT-A-SAME: {{^ }}vector{{$}}
      //          PRT-AO-NEXT: {{^ *}}// #pragma omp distribute simd{{$}}
      //           PRT-O-NEXT: {{^ *}}#pragma omp distribute simd{{$}}
      //          PRT-OA-NEXT: {{^ *}}// #pragma acc loop
      //      PRT-OA-SRC-SAME: {{^ }}INDEPENDENT GANG
      //  PRT-OA-AST-IND-SAME: {{^ }}independent
      // PRT-OA-AST-GANG-SAME: {{^ }}gang
      //          PRT-OA-SAME: {{^ }}vector{{$}}
      //             PRT-NEXT: for ({{.*}}) {
      //             PRT-NEXT:   {{printf|TGT_PRINTF}}
      //             PRT-NEXT:   k = 9999;
      //             PRT-NEXT: }
      //
      // Gang-partitioned mode executes each loop iteration once.
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      #pragma acc loop INDEPENDENT GANG vector
      for (int i = 0; i < 2; ++i) {
        TGT_PRINTF("loop iteration\n");
        k = 9999;
      }
      // Each gang that is assigned one or more i iterations will see the k from
      // the i loop above, and the remaining gangs will see the old k value.
      if (k == 999)
        save.writeInLaterLoopFoundOldVal = true;
      else if (k == 9999)
        save.writeInLaterLoopFoundNewVal = true;
      else
        save.writeInLaterLoopFoundBadVal = true;
    } // PRT: }

    // EXE-NEXT: save.loopOnlyFoundOldVal=1
    // EXE-NEXT: save.loopOnlyFoundBadVal=0
    // EXE-NEXT: save.loopOnlyArrFoundOldVal=1
    // EXE-NEXT: save.loopOnlyArrFoundBadVal=0
    // EXE-NEXT: save.assignLoopFoundBadVal=0
    // EXE-NEXT: save.writeInLaterLoopFoundOldVal=1
    // EXE-NEXT: save.writeInLaterLoopFoundNewVal=1
    // EXE-NEXT: save.writeInLaterLoopFoundBadVal=0
    // EXE-NEXT: loopOnly=88
    // EXE-NEXT: loopOnlyArr[0]=77
    printf("save.loopOnlyFoundOldVal=%d\n", save.loopOnlyFoundOldVal);
    printf("save.loopOnlyFoundBadVal=%d\n", save.loopOnlyFoundBadVal);
    printf("save.loopOnlyArrFoundOldVal=%d\n", save.loopOnlyArrFoundOldVal);
    printf("save.loopOnlyArrFoundBadVal=%d\n", save.loopOnlyArrFoundBadVal);
    printf("save.assignLoopFoundBadVal=%d\n", save.assignLoopFoundBadVal);
    printf("save.writeInLaterLoopFoundOldVal=%d\n", save.writeInLaterLoopFoundOldVal);
    printf("save.writeInLaterLoopFoundNewVal=%d\n", save.writeInLaterLoopFoundNewVal);
    printf("save.writeInLaterLoopFoundBadVal=%d\n", save.writeInLaterLoopFoundBadVal);
    printf("loopOnly=%d\n", loopOnly);
    printf("loopOnlyArr[0]=%d\n", loopOnlyArr[0]);
  }

  //............................................................................
  // acc loop INDEPENDENT GANG worker vector
  //............................................................................

  {
    int loopOnly;
    int loopOnlyArr[1];
    struct {
      bool loopOnlyFoundOldVal;
      bool loopOnlyFoundBadVal;

      bool loopOnlyArrFoundOldVal;
      bool loopOnlyArrFoundBadVal;

      bool assignLoopFoundBadVal;

      bool writeInLaterLoopFoundOldVal;
      bool writeInLaterLoopFoundNewVal;
      bool writeInLaterLoopFoundBadVal;
    } save;

    // DMP-LABEL: StringLiteral {{.*}} "acc loop INDEPENDENT GANG worker vector\n"
    // PRT-LABEL: printf("acc loop INDEPENDENT GANG worker vector\n");
    //   EXE-NOT: {{.}}
    // EXE-LABEL: acc loop INDEPENDENT GANG worker vector{{$}}
    //   EXE-NOT: {{.}}
    memset(&save, 0, sizeof(save));
    loopOnly = 88;
    loopOnlyArr[0] = 88;
    printf("acc loop INDEPENDENT GANG worker vector\n");

// FIXME: amdgcn misbehaves sometimes for worker loops.
// PRT-SRC-NEXT: #if !TGT_AMDGCN
#if !TGT_AMDGCN
    //      DMP: ACCParallelDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 3
    // DMP-NEXT:   ACCNomapClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnly' 'int'
    // DMP-NEXT:   ACCCopyClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'save'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'save'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    // DMP-NEXT:   ACCFirstprivateClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnly' 'int'
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 3
    // DMP-NEXT:     OMPMapClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'save'
    // DMP-NEXT:       DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    // DMP-NEXT:     OMPSharedClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'save'
    // DMP-NEXT:       DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    // DMP-NEXT:     OMPFirstprivateClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'loopOnly' 'int'
    //      DMP:     CompoundStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc parallel num_gangs(3){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(3) map(ompx_hold,tofrom: save,loopOnlyArr) shared(save,loopOnlyArr) firstprivate(loopOnly){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(3) map(ompx_hold,tofrom: save,loopOnlyArr) shared(save,loopOnlyArr) firstprivate(loopOnly){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(3){{$}}
    //    PRT-NEXT: {
    #pragma acc parallel num_gangs(3)
    {
      //              DMP: ACCLoopDirective
      //     DMP-IND-NEXT:   ACCIndependentClause
      //      DMP-IND-NOT:     <implicit>
      //    DMP-GANG-NEXT:   ACCGangClause
      //     DMP-GANG-NOT:     <implicit>
      //         DMP-NEXT:   ACCWorkerClause
      //          DMP-NOT:     <implicit>
      //         DMP-NEXT:   ACCVectorClause
      //          DMP-NOT:     <implicit>
      //  DMP-NO-IND-NEXT:   ACCIndependentClause {{.*}} <implicit>
      //         DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      //         DMP-NEXT:     DeclRefExpr {{.*}} 'save'
      //         DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnly' 'int'
      //         DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
      // DMP-NO-GANG-NEXT:   ACCGangClause {{.*}} <implicit>
      //         DMP-NEXT:   impl: OMPDistributeParallelForSimdDirective
      //         DMP-NEXT:     OMPSharedClause
      //          DMP-NOT:       <implicit>
      //         DMP-NEXT:       DeclRefExpr {{.*}} 'save'
      //         DMP-NEXT:       DeclRefExpr {{.*}} 'loopOnly' 'int'
      //         DMP-NEXT:       DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
      //              DMP:     ForStmt
      //
      //           PRT-A-NEXT: {{^ *}}#pragma acc loop
      //       PRT-A-SRC-SAME: {{^ }}INDEPENDENT GANG
      //   PRT-A-AST-IND-SAME: {{^ }}independent
      //  PRT-A-AST-GANG-SAME: {{^ }}gang
      //           PRT-A-SAME: {{^ }}worker vector{{$}}
      //          PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for simd shared(save,loopOnly,loopOnlyArr){{$}}
      //           PRT-O-NEXT: {{^ *}}#pragma omp distribute parallel for simd shared(save,loopOnly,loopOnlyArr){{$}}
      //          PRT-OA-NEXT: {{^ *}}// #pragma acc loop
      //      PRT-OA-SRC-SAME: {{^ }}INDEPENDENT GANG
      //  PRT-OA-AST-IND-SAME: {{^ }}independent
      // PRT-OA-AST-GANG-SAME: {{^ }}gang
      //          PRT-OA-SAME: {{^ }}worker vector{{$}}
      //             PRT-NEXT: for (int i = {{.*}}) {
      //             PRT-NEXT:   {{printf|TGT_PRINTF}}
      //                  PRT: }
      //
      // Gang-partitioned mode executes each loop iteration once.
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      #pragma acc loop INDEPENDENT GANG worker vector
      for (int i = 0; i < 2; ++i) {
        TGT_PRINTF("loop iteration\n");
        // We omit checking of an inner j loop this time.  j would be shared
        // among workers and vector lanes and thus would have an unpredictable
        // value.
        checkLoopPartShared(save, i, loopOnly, loopOnly, 88, 77);
        loopOnly = 77;
        checkLoopPartShared(save, i, loopOnlyArr, loopOnlyArr[0], 88, 77);
        loopOnlyArr[0] = 77;
      }

      // PRT: int k = 999;
      int k = 999;
      //              DMP: ACCLoopDirective
      //     DMP-IND-NEXT:   ACCIndependentClause
      //      DMP-IND-NOT:     <implicit>
      //    DMP-GANG-NEXT:   ACCGangClause
      //     DMP-GANG-NOT:     <implicit>
      //         DMP-NEXT:   ACCWorkerClause
      //          DMP-NOT:     <implicit>
      //         DMP-NEXT:   ACCVectorClause
      //          DMP-NOT:     <implicit>
      //  DMP-NO-IND-NEXT:   ACCIndependentClause {{.*}} <implicit>
      //         DMP-NEXT:   ACCPrivateClause {{.*}} <predetermined>
      //         DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
      // DMP-NO-GANG-NEXT:   ACCGangClause {{.*}} <implicit>
      //         DMP-NEXT:   impl: CompoundStmt
      //         DMP-NEXT:     DeclStmt
      //         DMP-NEXT:       VarDecl {{.*}} k 'int'
      //         DMP-NEXT:     OMPDistributeParallelForSimdDirective
      //          DMP-NOT:       OMPPrivateClause
      //              DMP:       ForStmt
      //
      //          PRT-AO-NEXT: // v----------ACC----------v
      //           PRT-A-NEXT: {{^ *}}#pragma acc loop
      //       PRT-A-SRC-SAME: {{^ }}INDEPENDENT GANG
      //   PRT-A-AST-IND-SAME: {{^ }}independent
      //  PRT-A-AST-GANG-SAME: {{^ }}gang
      //           PRT-A-SAME: {{^ }}worker vector{{$}}
      //           PRT-A-NEXT: for (k ={{.*}}) {
      //           PRT-A-NEXT:   {{print|TGT_PRINTF}}
      //           PRT-A-NEXT: }
      //          PRT-AO-NEXT: // ---------ACC->OMP--------
      //          PRT-AO-NEXT: // {
      //          PRT-AO-NEXT: //   int k;
      //          PRT-AO-NEXT: //   #pragma omp distribute parallel for simd{{$}}
      //          PRT-AO-NEXT: //   for (k ={{.*}}) {
      //          PRT-AO-NEXT: //     {{printf|TGT_PRINTF}}
      //          PRT-AO-NEXT: //   }
      //          PRT-AO-NEXT: // }
      //          PRT-AO-NEXT: // ^----------OMP----------^
      //
      //          PRT-OA-NEXT: // v----------OMP----------v
      //           PRT-O-NEXT: {
      //           PRT-O-NEXT:   int k;
      //           PRT-O-NEXT:   {{^ *}}#pragma omp distribute parallel for simd{{$}}
      //           PRT-O-NEXT:   for (k ={{.*}}) {
      //           PRT-O-NEXT:     {{printf|TGT_PRINTF}}
      //           PRT-O-NEXT:   }
      //           PRT-O-NEXT: }
      //          PRT-OA-NEXT: // ---------OMP<-ACC--------
      //          PRT-OA-NEXT: {{^ *}}// #pragma acc loop
      //      PRT-OA-SRC-SAME: {{^ }}INDEPENDENT GANG
      //  PRT-OA-AST-IND-SAME: {{^ }}independent
      // PRT-OA-AST-GANG-SAME: {{^ }}gang
      //          PRT-OA-SAME: {{^ }}worker vector{{$}}
      //          PRT-OA-NEXT: // for (k ={{.*}}) {
      //          PRT-OA-NEXT: //   {{printf|TGT_PRINTF}}
      //          PRT-OA-NEXT: // }
      //          PRT-OA-NEXT: // ^----------ACC----------^
      //
      // PRT-NOACC-NEXT: for (k ={{.*}}) {
      // PRT-NOACC-NEXT:   {{printf|TGT_PRINTF}}
      // PRT-NOACC-NEXT: }
      //
      // Gang-partitioned mode executes each loop iteration once.
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      #pragma acc loop INDEPENDENT GANG worker vector
      for (k = 1; k < 3; ++k) {
        TGT_PRINTF("loop iteration\n");
        // We omit checking of an inner l loop this time.  l would be shared
        // among workers and vector lanes and thus would have an unpredictable
        // value.
      }
      // Each gang will see the old k from before the loop above.
      if (k != 999)
        save.assignLoopFoundBadVal = true;

      // PRT: k = 999;
      k = 999;
      // k should have been dropped from the loop control variable list, so it
      // should now be implicitly shared in all cases.
      //
      //              DMP: ACCLoopDirective
      //     DMP-IND-NEXT:   ACCIndependentClause
      //      DMP-IND-NOT:     <implicit>
      //    DMP-GANG-NEXT:   ACCGangClause
      //     DMP-GANG-NOT:     <implicit>
      //         DMP-NEXT:   ACCWorkerClause
      //          DMP-NOT:     <implicit>
      //         DMP-NEXT:   ACCVectorClause
      //          DMP-NOT:     <implicit>
      //  DMP-NO-IND-NEXT:   ACCIndependentClause {{.*}} <implicit>
      //         DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      //         DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
      // DMP-NO-GANG-NEXT:   ACCGangClause {{.*}} <implicit>
      //         DMP-NEXT:   impl: OMPDistributeParallelForSimdDirective
      //         DMP-NEXT:     OMPSharedClause
      //          DMP-NOT:       <implicit>
      //         DMP-NEXT:       DeclRefExpr {{.*}} 'k' 'int'
      //              DMP:     ForStmt
      //
      //           PRT-A-NEXT: {{^ *}}#pragma acc loop
      //       PRT-A-SRC-SAME: {{^ }}INDEPENDENT GANG
      //   PRT-A-AST-IND-SAME: {{^ }}independent
      //  PRT-A-AST-GANG-SAME: {{^ }}gang
      //           PRT-A-SAME: {{^ }}worker vector{{$}}
      //          PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for simd shared(k){{$}}
      //           PRT-O-NEXT: {{^ *}}#pragma omp distribute parallel for simd shared(k){{$}}
      //          PRT-OA-NEXT: {{^ *}}// #pragma acc loop
      //      PRT-OA-SRC-SAME: {{^ }}INDEPENDENT GANG
      //  PRT-OA-AST-IND-SAME: {{^ }}independent
      // PRT-OA-AST-GANG-SAME: {{^ }}gang
      //          PRT-OA-SAME: {{^ }}worker vector{{$}}
      //             PRT-NEXT: for ({{.*}}) {
      //             PRT-NEXT:   {{printf|TGT_PRINTF}}
      //             PRT-NEXT:   k = 9999;
      //             PRT-NEXT: }
      //
      // Gang-partitioned mode executes each loop iteration once.
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      // EXE-TGT-USE-STDIO-NEXT: loop iteration
      #pragma acc loop INDEPENDENT GANG worker vector
      for (int i = 0; i < 2; ++i) {
        TGT_PRINTF("loop iteration\n");
        k = 9999;
      }
      // Each gang that is assigned one or more i iterations will see the k from
      // the i loop above, and the remaining gangs will see the old k value.
      if (k == 999)
        save.writeInLaterLoopFoundOldVal = true;
      else if (k == 9999)
        save.writeInLaterLoopFoundNewVal = true;
      else
        save.writeInLaterLoopFoundBadVal = true;
    } // PRT: }
#endif

    // EXE-NEXT: save.loopOnlyFoundOldVal=1
    // EXE-NEXT: save.loopOnlyFoundBadVal=0
    // EXE-NEXT: save.loopOnlyArrFoundOldVal=1
    // EXE-NEXT: save.loopOnlyArrFoundBadVal=0
    // EXE-NEXT: save.assignLoopFoundBadVal=0
    // EXE-NEXT: save.writeInLaterLoopFoundOldVal=1
    // EXE-NEXT: save.writeInLaterLoopFoundNewVal=1
    // EXE-NEXT: save.writeInLaterLoopFoundBadVal=0
    // EXE-NEXT: loopOnly=88
    // EXE-NEXT: loopOnlyArr[0]=77
// FIXME: amdgcn misbehaves sometimes for worker loops.
#if !TGT_AMDGCN
    printf("save.loopOnlyFoundOldVal=%d\n", save.loopOnlyFoundOldVal);
    printf("save.loopOnlyFoundBadVal=%d\n", save.loopOnlyFoundBadVal);
    printf("save.loopOnlyArrFoundOldVal=%d\n", save.loopOnlyArrFoundOldVal);
    printf("save.loopOnlyArrFoundBadVal=%d\n", save.loopOnlyArrFoundBadVal);
    printf("save.assignLoopFoundBadVal=%d\n", save.assignLoopFoundBadVal);
    printf("save.writeInLaterLoopFoundOldVal=%d\n", save.writeInLaterLoopFoundOldVal);
    printf("save.writeInLaterLoopFoundNewVal=%d\n", save.writeInLaterLoopFoundNewVal);
    printf("save.writeInLaterLoopFoundBadVal=%d\n", save.writeInLaterLoopFoundBadVal);
    printf("loopOnly=%d\n", loopOnly);
    printf("loopOnlyArr[0]=%d\n", loopOnlyArr[0]);
#else
    printf("save.loopOnlyFoundOldVal=%d\n", 1);
    printf("save.loopOnlyFoundBadVal=%d\n", 0);
    printf("save.loopOnlyArrFoundOldVal=%d\n", 1);
    printf("save.loopOnlyArrFoundBadVal=%d\n", 0);
    printf("save.assignLoopFoundBadVal=%d\n", 0);
    printf("save.writeInLaterLoopFoundOldVal=%d\n", 1);
    printf("save.writeInLaterLoopFoundNewVal=%d\n", 1);
    printf("save.writeInLaterLoopFoundBadVal=%d\n", 0);
    printf("loopOnly=%d\n", 88);
    printf("loopOnlyArr[0]=%d\n", 77);
#endif
  }

  //----------------------------------------------------------------------------
  // Repeat that but with acc parallel and acc loop combined.
  //
  // Then, j (loop control variable within acc loop with declared loop control
  // variable) cannot be within acc parallel and outside acc loop, so we declare
  // it outside acc parallel and check it inside acc loop.
  //
  // k and l (assigned acc loop control variable and inner loop control
  // variable) also cannot be within acc parallel and outside acc loop, so we
  // declare them outside acc parallel.  In these cases, we don't try to check
  // their runtime visibility.  Dumps and prints should be sufficient at this
  // point.
  //----------------------------------------------------------------------------

  //............................................................................
  // Sequential loops.
  //............................................................................

  {
    int loopOnly = 88;
    int loopOnlyArr[1] = {88};
    int j = 0;
    int k, l;
    struct {
      bool loopOnlyAtI0FoundBadVal;
      bool loopOnlyAtI1FoundBadVal;

      bool loopOnlyArrFoundOldVal;
      bool loopOnlyArrFoundBadVal;

      bool declNestedLoopFoundBadVal;
    } save;

    //. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 
    // acc parallel loop seq
    //. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 

    // DMP-LABEL: StringLiteral {{.*}} "declared loop var: acc parallel loop seq\n"
    // PRT-LABEL: printf("declared loop var: acc parallel loop seq\n");
    //   EXE-NOT: {{.}}
    // EXE-LABEL: declared loop var: acc parallel loop seq{{$}}
    //   EXE-NOT: {{.}}
    memset(&save, 0, sizeof(save));
    loopOnly = 88;
    loopOnlyArr[0] = 88;
    printf("declared loop var: acc parallel loop seq\n");
    //      DMP: ACCParallelLoopDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 3
    // DMP-NEXT:   ACCSeqClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:   effect: ACCParallelDirective
    // DMP-NEXT:     ACCNumGangsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 3
    // DMP-NEXT:     ACCNomapClause {{.*}} <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:       DeclRefExpr {{.*}} 'loopOnly' 'int'
    // DMP-NEXT:     ACCCopyClause {{.*}} <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'save'
    // DMP-NEXT:       DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    // DMP-NEXT:     ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'save'
    // DMP-NEXT:       DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    // DMP-NEXT:     ACCFirstprivateClause {{.*}} <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:       DeclRefExpr {{.*}} 'loopOnly' 'int'
    // DMP-NEXT:     impl: OMPTargetTeamsDirective
    // DMP-NEXT:       OMPNum_teamsClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 3
    // DMP-NEXT:       OMPMapClause
    //  DMP-NOT:         <implicit>
    // DMP-NEXT:         DeclRefExpr {{.*}} 'save'
    // DMP-NEXT:         DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    // DMP-NEXT:       OMPSharedClause
    //  DMP-NOT:         <implicit>
    // DMP-NEXT:         DeclRefExpr {{.*}} 'save'
    // DMP-NEXT:         DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    // DMP-NEXT:       OMPFirstprivateClause
    //  DMP-NOT:         <implicit>
    // DMP-NEXT:         DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:         DeclRefExpr {{.*}} 'loopOnly' 'int'
    //      DMP:     ACCLoopDirective
    // DMP-NEXT:       ACCSeqClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:         DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:         DeclRefExpr {{.*}} 'save'
    // DMP-NEXT:         DeclRefExpr {{.*}} 'loopOnly' 'int'
    // DMP-NEXT:         DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    // DMP-NEXT:       impl: ForStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(3) seq{{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(3) map(ompx_hold,tofrom: save,loopOnlyArr) shared(save,loopOnlyArr) firstprivate(j,loopOnly){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(3) map(ompx_hold,tofrom: save,loopOnlyArr) shared(save,loopOnlyArr) firstprivate(j,loopOnly){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(3) seq{{$}}
    //    PRT-NEXT: for ({{.*}}) {
    //         PRT: }
    //
    // Gang-redundant mode executes each loop iteration per gang.
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    #pragma acc parallel loop num_gangs(3) seq
    for (int i = 0; i < 2; ++i) {
      TGT_PRINTF("loop iteration\n");
      for (j = 0; j < 2; ++j)
        ;
      checkLoopSeqPrivate(save, i, loopOnly, 88, 77);
      loopOnly = 77;
      checkLoopSeqShared(save, i, loopOnlyArr, 88, 77);
      loopOnlyArr[0] = 77;
      // Each gang will see the final j from the j loop above.
      if (j != 2)
        save.declNestedLoopFoundBadVal = true;
    }
    // EXE-NEXT: save.loopOnlyAtI0FoundBadVal=0
    // EXE-NEXT: save.loopOnlyAtI1FoundBadVal=0
    // EXE-NEXT: save.loopOnlyArrFoundOldVal=1
    // EXE-NEXT: save.loopOnlyArrFoundBadVal=0
    // EXE-NEXT: save.declNestedLoopFoundBadVal=0
    // EXE-NEXT: loopOnly=88
    // EXE-NEXT: loopOnlyArr[0]=77
    printf("save.loopOnlyAtI0FoundBadVal=%d\n", save.loopOnlyAtI0FoundBadVal);
    printf("save.loopOnlyAtI1FoundBadVal=%d\n", save.loopOnlyAtI1FoundBadVal);
    printf("save.loopOnlyArrFoundOldVal=%d\n", save.loopOnlyArrFoundOldVal);
    printf("save.loopOnlyArrFoundBadVal=%d\n", save.loopOnlyArrFoundBadVal);
    printf("save.declNestedLoopFoundBadVal=%d\n", save.declNestedLoopFoundBadVal);
    printf("loopOnly=%d\n", loopOnly);
    printf("loopOnlyArr[0]=%d\n", loopOnlyArr[0]);

    // DMP-LABEL: StringLiteral {{.*}} "assigned loop var: acc parallel loop seq\n"
    // PRT-LABEL: printf("assigned loop var: acc parallel loop seq\n");
    //   EXE-NOT: {{.}}
    // EXE-LABEL: assigned loop var: acc parallel loop seq{{$}}
    //   EXE-NOT: {{.}}
    printf("assigned loop var: acc parallel loop seq\n");
    //      DMP: ACCParallelLoopDirective
    // DMP-NEXT:   ACCSeqClause
    //  DMP-NOT:   <implicit>
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 3
    // DMP-NEXT:   effect: ACCParallelDirective
    // DMP-NEXT:     ACCNumGangsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 3
    // DMP-NEXT:     ACCNomapClause {{.*}} <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'k' 'int'
    // DMP-NEXT:       DeclRefExpr {{.*}} 'l' 'int'
    // DMP-NEXT:     ACCFirstprivateClause {{.*}} <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'k' 'int'
    // DMP-NEXT:       DeclRefExpr {{.*}} 'l' 'int'
    // DMP-NEXT:     impl: OMPTargetTeamsDirective
    // DMP-NEXT:       OMPNum_teamsClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 3
    // DMP-NEXT:       OMPFirstprivateClause
    //  DMP-NOT:         <implicit>
    // DMP-NEXT:         DeclRefExpr {{.*}} 'k' 'int'
    // DMP-NEXT:         DeclRefExpr {{.*}} 'l' 'int'
    //      DMP:     ACCLoopDirective
    // DMP-NEXT:       ACCSeqClause
    // DMP-NEXT:       ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:         DeclRefExpr {{.*}} 'k' 'int'
    // DMP-NEXT:         DeclRefExpr {{.*}} 'l' 'int'
    // DMP-NEXT:       impl: ForStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc parallel loop seq num_gangs(3){{$}}
    // PRT-AO-NEXT: // #pragma omp target teams num_teams(3) firstprivate(k,l){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(3) firstprivate(k,l){{$}}
    // PRT-OA-NEXT: // #pragma acc parallel loop seq num_gangs(3){{$}}
    //    PRT-NEXT: for (k ={{.*}}) {
    //    PRT-NEXT:   {{printf|TGT_PRINTF}}
    //    PRT-NEXT:   for (l ={{.*}})
    //    PRT-NEXT:     ;
    //    PRT-NEXT: }
    //
    // Gang-redundant mode executes each loop iteration per gang.
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    #pragma acc parallel loop seq num_gangs(3)
    for (k = 1; k < 3; ++k) {
      TGT_PRINTF("loop iteration\n");
      for (l = 0; l < 2; ++l)
        ;
    }

    //. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 
    // acc parallel loop auto GANG
    //. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 

    // DMP-LABEL: StringLiteral {{.*}} "declared loop var: acc parallel loop auto GANG\n"
    // PRT-LABEL: printf("declared loop var: acc parallel loop auto GANG\n");
    //   EXE-NOT: {{.}}
    // EXE-LABEL: declared loop var: acc parallel loop auto GANG{{$}}
    //   EXE-NOT: {{.}}
    memset(&save, 0, sizeof(save));
    loopOnly = 88;
    loopOnlyArr[0] = 88;
    printf("declared loop var: acc parallel loop auto GANG\n");
    //           DMP: ACCParallelLoopDirective
    //      DMP-NEXT:   ACCNumGangsClause
    //      DMP-NEXT:     IntegerLiteral {{.*}} 'int' 3
    //      DMP-NEXT:   ACCAutoClause
    //       DMP-NOT:     <implicit>
    // DMP-GANG-NEXT:   ACCGangClause
    //  DMP-GANG-NOT:     <implicit>
    //      DMP-NEXT:   effect: ACCParallelDirective
    //      DMP-NEXT:     ACCNumGangsClause
    //      DMP-NEXT:       IntegerLiteral {{.*}} 'int' 3
    //      DMP-NEXT:     ACCNomapClause {{.*}} <implicit>
    //      DMP-NEXT:       DeclRefExpr {{.*}} 'j' 'int'
    //      DMP-NEXT:       DeclRefExpr {{.*}} 'loopOnly' 'int'
    //      DMP-NEXT:     ACCCopyClause {{.*}} <implicit>
    //      DMP-NEXT:       DeclRefExpr {{.*}} 'save'
    //      DMP-NEXT:       DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    //      DMP-NEXT:     ACCSharedClause {{.*}} <implicit>
    //      DMP-NEXT:       DeclRefExpr {{.*}} 'save'
    //      DMP-NEXT:       DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    //      DMP-NEXT:     ACCFirstprivateClause {{.*}} <implicit>
    //      DMP-NEXT:       DeclRefExpr {{.*}} 'j' 'int'
    //      DMP-NEXT:       DeclRefExpr {{.*}} 'loopOnly' 'int'
    //      DMP-NEXT:     impl: OMPTargetTeamsDirective
    //      DMP-NEXT:       OMPNum_teamsClause
    //      DMP-NEXT:         IntegerLiteral {{.*}} 'int' 3
    //      DMP-NEXT:       OMPMapClause
    //       DMP-NOT:         <implicit>
    //      DMP-NEXT:         DeclRefExpr {{.*}} 'save'
    //      DMP-NEXT:         DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    //      DMP-NEXT:       OMPSharedClause
    //       DMP-NOT:         <implicit>
    //      DMP-NEXT:         DeclRefExpr {{.*}} 'save'
    //      DMP-NEXT:         DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    //      DMP-NEXT:       OMPFirstprivateClause
    //       DMP-NOT:         <implicit>
    //      DMP-NEXT:         DeclRefExpr {{.*}} 'j' 'int'
    //      DMP-NEXT:         DeclRefExpr {{.*}} 'loopOnly' 'int'
    //           DMP:     ACCLoopDirective
    //      DMP-NEXT:       ACCAutoClause
    //       DMP-NOT:         <implicit>
    // DMP-GANG-NEXT:       ACCGangClause
    //  DMP-GANG-NOT:         <implicit>
    //      DMP-NEXT:       ACCSharedClause {{.*}} <implicit>
    //      DMP-NEXT:         DeclRefExpr {{.*}} 'j' 'int'
    //      DMP-NEXT:         DeclRefExpr {{.*}} 'save'
    //      DMP-NEXT:         DeclRefExpr {{.*}} 'loopOnly' 'int'
    //      DMP-NEXT:         DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    //      DMP-NEXT:       impl: ForStmt
    //
    //           PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(3) auto
    //       PRT-A-SRC-SAME: {{^ }}GANG
    //  PRT-A-AST-GANG-SAME: {{^ }}gang
    //           PRT-A-SAME: {{^$}}
    //          PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(3) map(ompx_hold,tofrom: save,loopOnlyArr) shared(save,loopOnlyArr) firstprivate(j,loopOnly){{$}}
    //           PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(3) map(ompx_hold,tofrom: save,loopOnlyArr) shared(save,loopOnlyArr) firstprivate(j,loopOnly){{$}}
    //          PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(3) auto
    //      PRT-OA-SRC-SAME: {{^ }}GANG
    // PRT-OA-AST-GANG-SAME: {{^ }}gang
    //          PRT-OA-SAME: {{^$}}
    //             PRT-NEXT: for ({{.*}}) {
    //                  PRT: }
    //
    // Gang-redundant mode executes each loop iteration per gang.
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    #pragma acc parallel loop num_gangs(3) auto GANG
    for (int i = 0; i < 2; ++i) {
      TGT_PRINTF("loop iteration\n");
      for (j = 0; j < 2; ++j)
        ;
      checkLoopSeqPrivate(save, i, loopOnly, 88, 77);
      loopOnly = 77;
      checkLoopSeqShared(save, i, loopOnlyArr, 88, 77);
      loopOnlyArr[0] = 77;
      // Each gang will see the final j from the j loop above.
      if (j != 2)
        save.declNestedLoopFoundBadVal = true;
    }
    // EXE-NEXT: save.loopOnlyAtI0FoundBadVal=0
    // EXE-NEXT: save.loopOnlyAtI1FoundBadVal=0
    // EXE-NEXT: save.loopOnlyArrFoundOldVal=1
    // EXE-NEXT: save.loopOnlyArrFoundBadVal=0
    // EXE-NEXT: save.declNestedLoopFoundBadVal=0
    // EXE-NEXT: loopOnly=88
    // EXE-NEXT: loopOnlyArr[0]=77
    printf("save.loopOnlyAtI0FoundBadVal=%d\n", save.loopOnlyAtI0FoundBadVal);
    printf("save.loopOnlyAtI1FoundBadVal=%d\n", save.loopOnlyAtI1FoundBadVal);
    printf("save.loopOnlyArrFoundOldVal=%d\n", save.loopOnlyArrFoundOldVal);
    printf("save.loopOnlyArrFoundBadVal=%d\n", save.loopOnlyArrFoundBadVal);
    printf("save.declNestedLoopFoundBadVal=%d\n", save.declNestedLoopFoundBadVal);
    printf("loopOnly=%d\n", loopOnly);
    printf("loopOnlyArr[0]=%d\n", loopOnlyArr[0]);

    // DMP-LABEL: StringLiteral {{.*}} "assigned loop var: acc parallel loop auto GANG\n"
    // PRT-LABEL: printf("assigned loop var: acc parallel loop auto GANG\n");
    //   EXE-NOT: {{.}}
    // EXE-LABEL: assigned loop var: acc parallel loop auto GANG{{$}}
    //   EXE-NOT: {{.}}
    printf("assigned loop var: acc parallel loop auto GANG\n");
    //           DMP: ACCParallelLoopDirective
    //      DMP-NEXT:   ACCAutoClause
    //       DMP-NOT:     <implicit>
    // DMP-GANG-NEXT:   ACCGangClause
    //  DMP-GANG-NOT:     <implicit>
    //      DMP-NEXT:   ACCNumGangsClause
    //      DMP-NEXT:     IntegerLiteral {{.*}} 'int' 3
    //      DMP-NEXT:   effect: ACCParallelDirective
    //      DMP-NEXT:     ACCNumGangsClause
    //      DMP-NEXT:       IntegerLiteral {{.*}} 'int' 3
    //      DMP-NEXT:     ACCNomapClause {{.*}} <implicit>
    //      DMP-NEXT:       DeclRefExpr {{.*}} 'l' 'int'
    //      DMP-NEXT:     ACCFirstprivateClause {{.*}} <implicit>
    //      DMP-NEXT:       DeclRefExpr {{.*}} 'l' 'int'
    //      DMP-NEXT:     impl: OMPTargetTeamsDirective
    //      DMP-NEXT:       OMPNum_teamsClause
    //      DMP-NEXT:         IntegerLiteral {{.*}} 'int' 3
    //      DMP-NEXT:       OMPFirstprivateClause
    //       DMP-NOT:         <implicit>
    //      DMP-NEXT:         DeclRefExpr {{.*}} 'l' 'int'
    //           DMP:     ACCLoopDirective
    //      DMP-NEXT:       ACCAutoClause
    //       DMP-NOT:         <implicit>
    // DMP-GANG-NEXT:       ACCGangClause
    //  DMP-GANG-NOT:         <implicit>
    //      DMP-NEXT:       ACCPrivateClause {{.*}} <predetermined>
    //      DMP-NEXT:         DeclRefExpr {{.*}} 'k' 'int'
    //      DMP-NEXT:       ACCSharedClause {{.*}} <implicit>
    //      DMP-NEXT:         DeclRefExpr {{.*}} 'l' 'int'
    //      DMP-NEXT:       impl: CompoundStmt
    //      DMP-NEXT:         DeclStmt
    //      DMP-NEXT:           VarDecl {{.*}} k 'int'
    //      DMP-NEXT:         ForStmt
    //
    //         PRT-AO-NEXT: // v----------ACC----------v
    //          PRT-A-NEXT: {{^ *}}#pragma acc parallel loop auto
    //      PRT-A-SRC-SAME: {{^ }}GANG
    // PRT-A-AST-GANG-SAME: {{^ }}gang
    //          PRT-A-SAME: {{^ }}num_gangs(3){{$}}
    //          PRT-A-NEXT: for (k ={{.*}}) {
    //          PRT-A-NEXT:   {{printf|TGT_PRINTF}}
    //          PRT-A-NEXT:   for (l ={{.*}})
    //          PRT-A-NEXT:     ;
    //          PRT-A-NEXT: }
    //         PRT-AO-NEXT: // ---------ACC->OMP--------
    //         PRT-AO-NEXT: // #pragma omp target teams num_teams(3) firstprivate(l){{$}}
    //         PRT-AO-NEXT: // {
    //         PRT-AO-NEXT: //   int k;
    //         PRT-AO-NEXT: //   for (k ={{.*}}) {
    //         PRT-AO-NEXT: //     {{printf|TGT_PRINTF}}
    //         PRT-AO-NEXT: //     for (l ={{.*}})
    //         PRT-AO-NEXT: //       ;
    //         PRT-AO-NEXT: //   }
    //         PRT-AO-NEXT: // }
    //         PRT-AO-NEXT: // ^----------OMP----------^
    //
    //          PRT-OA-NEXT: // v----------OMP----------v
    //           PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(3) firstprivate(l){{$}}
    //           PRT-O-NEXT: {
    //           PRT-O-NEXT:   int k;
    //           PRT-O-NEXT:   for (k ={{.*}}) {
    //           PRT-O-NEXT:     {{printf|TGT_PRINTF}}
    //           PRT-O-NEXT:     for (l ={{.*}})
    //           PRT-O-NEXT:       ;
    //           PRT-O-NEXT:   }
    //           PRT-O-NEXT: }
    //          PRT-OA-NEXT: // ---------OMP<-ACC--------
    //          PRT-OA-NEXT: // #pragma acc parallel loop auto
    //      PRT-OA-SRC-SAME: {{^ }}GANG
    // PRT-OA-AST-GANG-SAME: {{^ }}gang
    //          PRT-OA-SAME: {{^ }}num_gangs(3){{$}}
    //          PRT-OA-NEXT: // for (k ={{.*}}) {
    //          PRT-OA-NEXT: //   {{printf|TGT_PRINTF}}
    //          PRT-OA-NEXT: //   for (l ={{.*}})
    //          PRT-OA-NEXT: //     ;
    //          PRT-OA-NEXT: // }
    //          PRT-OA-NEXT: // ^----------ACC----------^
    //
    // PRT-NOACC-NEXT: for (k ={{.*}}) {
    // PRT-NOACC-NEXT:   {{printf|TGT_PRINTF}}
    // PRT-NOACC-NEXT:   for (l ={{.*}})
    // PRT-NOACC-NEXT:     ;
    // PRT-NOACC-NEXT: }
    //
    // Gang-redundant mode executes each loop iteration per gang.
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    #pragma acc parallel loop auto GANG num_gangs(3)
    for (k = 1; k < 3; ++k) {
      TGT_PRINTF("loop iteration\n");
      for (l = 0; l < 2; ++l)
        ;
    }

    //. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 
    // acc parallel loop auto GANG worker
    //. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 

    // DMP-LABEL: StringLiteral {{.*}} "declare loop var: acc parallel loop auto GANG worker\n"
    // PRT-LABEL: printf("declare loop var: acc parallel loop auto GANG worker\n");
    //   EXE-NOT: {{.}}
    // EXE-LABEL: declare loop var: acc parallel loop auto GANG worker{{$}}
    //   EXE-NOT: {{.}}
    memset(&save, 0, sizeof(save));
    loopOnly = 88;
    loopOnlyArr[0] = 88;
    printf("declare loop var: acc parallel loop auto GANG worker\n");
    //           DMP: ACCParallelLoopDirective
    //      DMP-NEXT:   ACCNumGangsClause
    //      DMP-NEXT:     IntegerLiteral {{.*}} 'int' 3
    //      DMP-NEXT:   ACCAutoClause
    //       DMP-NOT:     <implicit>
    // DMP-GANG-NEXT:   ACCGangClause
    //  DMP-GANG-NOT:     <implicit>
    //      DMP-NEXT:   ACCWorkerClause
    //       DMP-NOT:     <implicit>
    //      DMP-NEXT:   effect: ACCParallelDirective
    //      DMP-NEXT:     ACCNumGangsClause
    //      DMP-NEXT:       IntegerLiteral {{.*}} 'int' 3
    //      DMP-NEXT:     ACCNomapClause {{.*}} <implicit>
    //      DMP-NEXT:       DeclRefExpr {{.*}} 'j' 'int'
    //      DMP-NEXT:       DeclRefExpr {{.*}} 'loopOnly' 'int'
    //      DMP-NEXT:     ACCCopyClause {{.*}} <implicit>
    //      DMP-NEXT:       DeclRefExpr {{.*}} 'save'
    //      DMP-NEXT:       DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    //      DMP-NEXT:     ACCSharedClause {{.*}} <implicit>
    //      DMP-NEXT:       DeclRefExpr {{.*}} 'save'
    //      DMP-NEXT:       DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    //      DMP-NEXT:     ACCFirstprivateClause {{.*}} <implicit>
    //      DMP-NEXT:       DeclRefExpr {{.*}} 'j' 'int'
    //      DMP-NEXT:       DeclRefExpr {{.*}} 'loopOnly' 'int'
    //      DMP-NEXT:     impl: OMPTargetTeamsDirective
    //      DMP-NEXT:       OMPNum_teamsClause
    //      DMP-NEXT:         IntegerLiteral {{.*}} 'int' 3
    //      DMP-NEXT:       OMPMapClause
    //       DMP-NOT:         <implicit>
    //      DMP-NEXT:         DeclRefExpr {{.*}} 'save'
    //      DMP-NEXT:         DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    //      DMP-NEXT:       OMPSharedClause
    //       DMP-NOT:         <implicit>
    //      DMP-NEXT:         DeclRefExpr {{.*}} 'save'
    //      DMP-NEXT:         DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    //      DMP-NEXT:       OMPFirstprivateClause
    //       DMP-NOT:         <implicit>
    //      DMP-NEXT:         DeclRefExpr {{.*}} 'j' 'int'
    //      DMP-NEXT:         DeclRefExpr {{.*}} 'loopOnly' 'int'
    //           DMP:     ACCLoopDirective
    //      DMP-NEXT:       ACCAutoClause
    //       DMP-NOT:         <implicit>
    // DMP-GANG-NEXT:       ACCGangClause
    //  DMP-GANG-NOT:         <implicit>
    //      DMP-NEXT:       ACCWorkerClause
    //       DMP-NOT:         <implicit>
    //      DMP-NEXT:       ACCSharedClause {{.*}} <implicit>
    //      DMP-NEXT:         DeclRefExpr {{.*}} 'j' 'int'
    //      DMP-NEXT:         DeclRefExpr {{.*}} 'save'
    //      DMP-NEXT:         DeclRefExpr {{.*}} 'loopOnly' 'int'
    //      DMP-NEXT:         DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    //      DMP-NEXT:       impl: ForStmt
    //
    //           PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(3) auto
    //       PRT-A-SRC-SAME: {{^ }}GANG
    //  PRT-A-AST-GANG-SAME: {{^ }}gang
    //           PRT-A-SAME: {{^ }}worker{{$}}
    //          PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(3) map(ompx_hold,tofrom: save,loopOnlyArr) shared(save,loopOnlyArr) firstprivate(j,loopOnly){{$}}
    //           PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(3) map(ompx_hold,tofrom: save,loopOnlyArr) shared(save,loopOnlyArr) firstprivate(j,loopOnly){{$}}
    //          PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(3) auto
    //      PRT-OA-SRC-SAME: {{^ }}GANG
    // PRT-OA-AST-GANG-SAME: {{^ }}gang
    //          PRT-OA-SAME: {{^ }}worker{{$}}
    //             PRT-NEXT: for ({{.*}}) {
    //                  PRT: }
    //
    // Gang-redundant mode executes each loop iteration per gang.
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    #pragma acc parallel loop num_gangs(3) auto GANG worker
    for (int i = 0; i < 2; ++i) {
      TGT_PRINTF("loop iteration\n");
      for (j = 0; j < 2; ++j)
        ;
      checkLoopSeqPrivate(save, i, loopOnly, 88, 77);
      loopOnly = 77;
      checkLoopSeqShared(save, i, loopOnlyArr, 88, 77);
      loopOnlyArr[0] = 77;
      // Each gang will see the final j from the j loop above.
      if (j != 2)
        save.declNestedLoopFoundBadVal = true;
    }
    // EXE-NEXT: save.loopOnlyAtI0FoundBadVal=0
    // EXE-NEXT: save.loopOnlyAtI1FoundBadVal=0
    // EXE-NEXT: save.loopOnlyArrFoundOldVal=1
    // EXE-NEXT: save.loopOnlyArrFoundBadVal=0
    // EXE-NEXT: save.declNestedLoopFoundBadVal=0
    // EXE-NEXT: loopOnly=88
    // EXE-NEXT: loopOnlyArr[0]=77
    printf("save.loopOnlyAtI0FoundBadVal=%d\n", save.loopOnlyAtI0FoundBadVal);
    printf("save.loopOnlyAtI1FoundBadVal=%d\n", save.loopOnlyAtI1FoundBadVal);
    printf("save.loopOnlyArrFoundOldVal=%d\n", save.loopOnlyArrFoundOldVal);
    printf("save.loopOnlyArrFoundBadVal=%d\n", save.loopOnlyArrFoundBadVal);
    printf("save.declNestedLoopFoundBadVal=%d\n", save.declNestedLoopFoundBadVal);
    printf("loopOnly=%d\n", loopOnly);
    printf("loopOnlyArr[0]=%d\n", loopOnlyArr[0]);

    // DMP-LABEL: StringLiteral {{.*}} "assigned loop var: acc parallel loop auto GANG worker\n"
    // PRT-LABEL: printf("assigned loop var: acc parallel loop auto GANG worker\n");
    //   EXE-NOT: {{.}}
    // EXE-LABEL: assigned loop var: acc parallel loop auto GANG worker{{$}}
    //   EXE-NOT: {{.}}
    printf("assigned loop var: acc parallel loop auto GANG worker\n");
    //           DMP: ACCParallelLoopDirective
    //      DMP-NEXT:   ACCAutoClause
    //       DMP-NOT:     <implicit>
    // DMP-GANG-NEXT:   ACCGangClause
    //  DMP-GANG-NOT:     <implicit>
    //      DMP-NEXT:   ACCWorkerClause
    //       DMP-NOT:     <implicit>
    //      DMP-NEXT:   ACCNumGangsClause
    //      DMP-NEXT:     IntegerLiteral {{.*}} 'int' 3
    //      DMP-NEXT:   effect: ACCParallelDirective
    //      DMP-NEXT:     ACCNumGangsClause
    //      DMP-NEXT:       IntegerLiteral {{.*}} 'int' 3
    //      DMP-NEXT:     ACCNomapClause {{.*}} <implicit>
    //      DMP-NEXT:       DeclRefExpr {{.*}} 'l' 'int'
    //      DMP-NEXT:     ACCFirstprivateClause {{.*}} <implicit>
    //      DMP-NEXT:       DeclRefExpr {{.*}} 'l' 'int'
    //      DMP-NEXT:     impl: OMPTargetTeamsDirective
    //      DMP-NEXT:       OMPNum_teamsClause
    //      DMP-NEXT:         IntegerLiteral {{.*}} 'int' 3
    //      DMP-NEXT:       OMPFirstprivateClause
    //       DMP-NOT:         <implicit>
    //      DMP-NEXT:         DeclRefExpr {{.*}} 'l' 'int'
    //           DMP:     ACCLoopDirective
    //      DMP-NEXT:       ACCAutoClause
    //       DMP-NOT:         <implicit>
    // DMP-GANG-NEXT:       ACCGangClause
    //  DMP-GANG-NOT:         <implicit>
    //      DMP-NEXT:       ACCWorkerClause
    //       DMP-NOT:         <implicit>
    //      DMP-NEXT:       ACCPrivateClause {{.*}} <predetermined>
    //      DMP-NEXT:         DeclRefExpr {{.*}} 'k' 'int'
    //      DMP-NEXT:       ACCSharedClause {{.*}} <implicit>
    //      DMP-NEXT:         DeclRefExpr {{.*}} 'l' 'int'
    //      DMP-NEXT:       impl: CompoundStmt
    //      DMP-NEXT:         DeclStmt
    //      DMP-NEXT:           VarDecl {{.*}} k 'int'
    //      DMP-NEXT:         ForStmt
    //
    //         PRT-AO-NEXT: // v----------ACC----------v
    //          PRT-A-NEXT: {{^ *}}#pragma acc parallel loop auto
    //      PRT-A-SRC-SAME: {{^ }}GANG
    // PRT-A-AST-GANG-SAME: {{^ }}gang
    //          PRT-A-SAME: {{^ }}worker num_gangs(3){{$}}
    //          PRT-A-NEXT: for (k ={{.*}}) {
    //          PRT-A-NEXT:   {{printf|TGT_PRINTF}}
    //          PRT-A-NEXT:   for (l ={{.*}})
    //          PRT-A-NEXT:     ;
    //          PRT-A-NEXT: }
    //         PRT-AO-NEXT: // ---------ACC->OMP--------
    //         PRT-AO-NEXT: // #pragma omp target teams num_teams(3) firstprivate(l){{$}}
    //         PRT-AO-NEXT: // {
    //         PRT-AO-NEXT: //   int k;
    //         PRT-AO-NEXT: //   for (k ={{.*}}) {
    //         PRT-AO-NEXT: //     {{printf|TGT_PRINTF}}
    //         PRT-AO-NEXT: //     for (l ={{.*}})
    //         PRT-AO-NEXT: //       ;
    //         PRT-AO-NEXT: //   }
    //         PRT-AO-NEXT: // }
    //         PRT-AO-NEXT: // ^----------OMP----------^
    //
    //          PRT-OA-NEXT: // v----------OMP----------v
    //           PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(3) firstprivate(l){{$}}
    //           PRT-O-NEXT: {
    //           PRT-O-NEXT:   int k;
    //           PRT-O-NEXT:   for (k ={{.*}}) {
    //           PRT-O-NEXT:     {{printf|TGT_PRINTF}}
    //           PRT-O-NEXT:     for (l ={{.*}})
    //           PRT-O-NEXT:       ;
    //           PRT-O-NEXT:   }
    //           PRT-O-NEXT: }
    //          PRT-OA-NEXT: // ---------OMP<-ACC--------
    //          PRT-OA-NEXT: // #pragma acc parallel loop auto
    //      PRT-OA-SRC-SAME: {{^ }}GANG
    // PRT-OA-AST-GANG-SAME: {{^ }}gang
    //          PRT-OA-SAME: {{^ }}worker num_gangs(3){{$}}
    //          PRT-OA-NEXT: // for (k ={{.*}}) {
    //          PRT-OA-NEXT: //   {{printf|TGT_PRINTF}}
    //          PRT-OA-NEXT: //   for (l ={{.*}})
    //          PRT-OA-NEXT: //     ;
    //          PRT-OA-NEXT: // }
    //          PRT-OA-NEXT: // ^----------ACC----------^
    //
    // PRT-NOACC-NEXT: for (k ={{.*}}) {
    // PRT-NOACC-NEXT:   {{printf|TGT_PRINTF}}
    // PRT-NOACC-NEXT:   for (l ={{.*}})
    // PRT-NOACC-NEXT:     ;
    // PRT-NOACC-NEXT: }
    //
    // Gang-redundant mode executes each loop iteration per gang.
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    #pragma acc parallel loop auto GANG worker num_gangs(3)
    for (k = 1; k < 3; ++k) {
      TGT_PRINTF("loop iteration\n");
      for (l = 0; l < 2; ++l)
        ;
    }

    //. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 
    // acc parallel loop auto GANG vector
    //. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 

    // DMP-LABEL: StringLiteral {{.*}} "declared loop var: acc parallel loop auto GANG vector\n"
    // PRT-LABEL: printf("declared loop var: acc parallel loop auto GANG vector\n");
    //   EXE-NOT: {{.}}
    // EXE-LABEL: declared loop var: acc parallel loop auto GANG vector{{$}}
    //   EXE-NOT: {{.}}
    memset(&save, 0, sizeof(save));
    loopOnly = 88;
    loopOnlyArr[0] = 88;
    printf("declared loop var: acc parallel loop auto GANG vector\n");
    //           DMP: ACCParallelLoopDirective
    //      DMP-NEXT:   ACCNumGangsClause
    //      DMP-NEXT:     IntegerLiteral {{.*}} 'int' 3
    //      DMP-NEXT:   ACCAutoClause
    //       DMP-NOT:     <implicit>
    // DMP-GANG-NEXT:   ACCGangClause
    //  DMP-GANG-NOT:     <implicit>
    //      DMP-NEXT:   ACCVectorClause
    //       DMP-NOT:     <implicit>
    //      DMP-NEXT:   effect: ACCParallelDirective
    //      DMP-NEXT:     ACCNumGangsClause
    //      DMP-NEXT:       IntegerLiteral {{.*}} 'int' 3
    //      DMP-NEXT:     ACCNomapClause {{.*}} <implicit>
    //      DMP-NEXT:       DeclRefExpr {{.*}} 'j' 'int'
    //      DMP-NEXT:       DeclRefExpr {{.*}} 'loopOnly' 'int'
    //      DMP-NEXT:     ACCCopyClause {{.*}} <implicit>
    //      DMP-NEXT:       DeclRefExpr {{.*}} 'save'
    //      DMP-NEXT:       DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    //      DMP-NEXT:     ACCSharedClause {{.*}} <implicit>
    //      DMP-NEXT:       DeclRefExpr {{.*}} 'save'
    //      DMP-NEXT:       DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    //      DMP-NEXT:     ACCFirstprivateClause {{.*}} <implicit>
    //      DMP-NEXT:       DeclRefExpr {{.*}} 'j' 'int'
    //      DMP-NEXT:       DeclRefExpr {{.*}} 'loopOnly' 'int'
    //      DMP-NEXT:     impl: OMPTargetTeamsDirective
    //      DMP-NEXT:       OMPNum_teamsClause
    //      DMP-NEXT:         IntegerLiteral {{.*}} 'int' 3
    //      DMP-NEXT:       OMPMapClause
    //       DMP-NOT:         <implicit>
    //      DMP-NEXT:         DeclRefExpr {{.*}} 'save'
    //      DMP-NEXT:         DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    //      DMP-NEXT:       OMPSharedClause
    //       DMP-NOT:         <implicit>
    //      DMP-NEXT:         DeclRefExpr {{.*}} 'save'
    //      DMP-NEXT:         DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    //      DMP-NEXT:       OMPFirstprivateClause
    //       DMP-NOT:         <implicit>
    //      DMP-NEXT:         DeclRefExpr {{.*}} 'j' 'int'
    //      DMP-NEXT:         DeclRefExpr {{.*}} 'loopOnly' 'int'
    //           DMP:     ACCLoopDirective
    //      DMP-NEXT:       ACCAutoClause
    //       DMP-NOT:         <implicit>
    // DMP-GANG-NEXT:       ACCGangClause
    //  DMP-GANG-NOT:         <implicit>
    //      DMP-NEXT:       ACCVectorClause
    //       DMP-NOT:         <implicit>
    //      DMP-NEXT:       ACCSharedClause {{.*}} <implicit>
    //      DMP-NEXT:         DeclRefExpr {{.*}} 'j' 'int'
    //      DMP-NEXT:         DeclRefExpr {{.*}} 'save'
    //      DMP-NEXT:         DeclRefExpr {{.*}} 'loopOnly' 'int'
    //      DMP-NEXT:         DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    //      DMP-NEXT:       impl: ForStmt
    //
    //           PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(3) auto
    //       PRT-A-SRC-SAME: {{^ }}GANG
    //  PRT-A-AST-GANG-SAME: {{^ }}gang
    //           PRT-A-SAME: {{^ }}vector{{$}}
    //          PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(3) map(ompx_hold,tofrom: save,loopOnlyArr) shared(save,loopOnlyArr) firstprivate(j,loopOnly){{$}}
    //           PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(3) map(ompx_hold,tofrom: save,loopOnlyArr) shared(save,loopOnlyArr) firstprivate(j,loopOnly){{$}}
    //          PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(3) auto
    //      PRT-OA-SRC-SAME: {{^ }}GANG
    // PRT-OA-AST-GANG-SAME: {{^ }}gang
    //          PRT-OA-SAME: {{^ }}vector{{$}}
    //             PRT-NEXT: for ({{.*}}) {
    //                  PRT: }
    //
    // Gang-redundant mode executes each loop iteration per gang.
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    #pragma acc parallel loop num_gangs(3) auto GANG vector
    for (int i = 0; i < 2; ++i) {
      TGT_PRINTF("loop iteration\n");
      for (j = 0; j < 2; ++j)
        ;
      checkLoopSeqPrivate(save, i, loopOnly, 88, 77);
      loopOnly = 77;
      checkLoopSeqShared(save, i, loopOnlyArr, 88, 77);
      loopOnlyArr[0] = 77;
      // Each gang will see the final j from the j loop above.
      if (j != 2)
        save.declNestedLoopFoundBadVal = true;
    }
    // EXE-NEXT: save.loopOnlyAtI0FoundBadVal=0
    // EXE-NEXT: save.loopOnlyAtI1FoundBadVal=0
    // EXE-NEXT: save.loopOnlyArrFoundOldVal=1
    // EXE-NEXT: save.loopOnlyArrFoundBadVal=0
    // EXE-NEXT: save.declNestedLoopFoundBadVal=0
    // EXE-NEXT: loopOnly=88
    // EXE-NEXT: loopOnlyArr[0]=77
    printf("save.loopOnlyAtI0FoundBadVal=%d\n", save.loopOnlyAtI0FoundBadVal);
    printf("save.loopOnlyAtI1FoundBadVal=%d\n", save.loopOnlyAtI1FoundBadVal);
    printf("save.loopOnlyArrFoundOldVal=%d\n", save.loopOnlyArrFoundOldVal);
    printf("save.loopOnlyArrFoundBadVal=%d\n", save.loopOnlyArrFoundBadVal);
    printf("save.declNestedLoopFoundBadVal=%d\n", save.declNestedLoopFoundBadVal);
    printf("loopOnly=%d\n", loopOnly);
    printf("loopOnlyArr[0]=%d\n", loopOnlyArr[0]);

    // DMP-LABEL: StringLiteral {{.*}} "assigned loop var: acc parallel loop auto GANG vector\n"
    // PRT-LABEL: printf("assigned loop var: acc parallel loop auto GANG vector\n");
    //   EXE-NOT: {{.}}
    // EXE-LABEL: assigned loop var: acc parallel loop auto GANG vector{{$}}
    //   EXE-NOT: {{.}}
    printf("assigned loop var: acc parallel loop auto GANG vector\n");
    //           DMP: ACCParallelLoopDirective
    //      DMP-NEXT:   ACCAutoClause
    //       DMP-NOT:     <implicit>
    // DMP-GANG-NEXT:   ACCGangClause
    //  DMP-GANG-NOT:     <implicit>
    //      DMP-NEXT:   ACCVectorClause
    //       DMP-NOT:     <implicit>
    //      DMP-NEXT:   ACCNumGangsClause
    //      DMP-NEXT:     IntegerLiteral {{.*}} 'int' 3
    //      DMP-NEXT:   effect: ACCParallelDirective
    //      DMP-NEXT:     ACCNumGangsClause
    //      DMP-NEXT:       IntegerLiteral {{.*}} 'int' 3
    //      DMP-NEXT:     ACCNomapClause {{.*}} <implicit>
    //      DMP-NEXT:       DeclRefExpr {{.*}} 'l' 'int'
    //      DMP-NEXT:     ACCFirstprivateClause {{.*}} <implicit>
    //      DMP-NEXT:       DeclRefExpr {{.*}} 'l' 'int'
    //      DMP-NEXT:     impl: OMPTargetTeamsDirective
    //      DMP-NEXT:       OMPNum_teamsClause
    //      DMP-NEXT:         IntegerLiteral {{.*}} 'int' 3
    //      DMP-NEXT:       OMPFirstprivateClause
    //       DMP-NOT:         <implicit>
    //      DMP-NEXT:         DeclRefExpr {{.*}} 'l' 'int'
    //           DMP:     ACCLoopDirective
    //      DMP-NEXT:       ACCAutoClause
    //       DMP-NOT:         <implicit>
    // DMP-GANG-NEXT:       ACCGangClause
    //  DMP-GANG-NOT:         <implicit>
    //      DMP-NEXT:       ACCVectorClause
    //       DMP-NOT:         <implicit>
    //      DMP-NEXT:       ACCPrivateClause {{.*}} <predetermined>
    //      DMP-NEXT:         DeclRefExpr {{.*}} 'k' 'int'
    //      DMP-NEXT:       ACCSharedClause {{.*}} <implicit>
    //      DMP-NEXT:         DeclRefExpr {{.*}} 'l' 'int'
    //      DMP-NEXT:       impl: CompoundStmt
    //      DMP-NEXT:         DeclStmt
    //      DMP-NEXT:           VarDecl {{.*}} k 'int'
    //      DMP-NEXT:         ForStmt
    //
    //         PRT-AO-NEXT: // v----------ACC----------v
    //          PRT-A-NEXT: {{^ *}}#pragma acc parallel loop auto
    //      PRT-A-SRC-SAME: {{^ }}GANG
    // PRT-A-AST-GANG-SAME: {{^ }}gang
    //          PRT-A-SAME: {{^ }}vector num_gangs(3){{$}}
    //          PRT-A-NEXT: for (k ={{.*}}) {
    //          PRT-A-NEXT:   {{printf|TGT_PRINTF}}
    //          PRT-A-NEXT:   for (l ={{.*}})
    //          PRT-A-NEXT:     ;
    //          PRT-A-NEXT: }
    //         PRT-AO-NEXT: // ---------ACC->OMP--------
    //         PRT-AO-NEXT: // #pragma omp target teams num_teams(3) firstprivate(l){{$}}
    //         PRT-AO-NEXT: // {
    //         PRT-AO-NEXT: //   int k;
    //         PRT-AO-NEXT: //   for (k ={{.*}}) {
    //         PRT-AO-NEXT: //     {{printf|TGT_PRINTF}}
    //         PRT-AO-NEXT: //     for (l ={{.*}})
    //         PRT-AO-NEXT: //       ;
    //         PRT-AO-NEXT: //   }
    //         PRT-AO-NEXT: // }
    //         PRT-AO-NEXT: // ^----------OMP----------^
    //
    //          PRT-OA-NEXT: // v----------OMP----------v
    //           PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(3) firstprivate(l){{$}}
    //           PRT-O-NEXT: {
    //           PRT-O-NEXT:   int k;
    //           PRT-O-NEXT:   for (k ={{.*}}) {
    //           PRT-O-NEXT:     {{printf|TGT_PRINTF}}
    //           PRT-O-NEXT:     for (l ={{.*}})
    //           PRT-O-NEXT:       ;
    //           PRT-O-NEXT:   }
    //           PRT-O-NEXT: }
    //          PRT-OA-NEXT: // ---------OMP<-ACC--------
    //          PRT-OA-NEXT: // #pragma acc parallel loop auto
    //      PRT-OA-SRC-SAME: {{^ }}GANG
    // PRT-OA-AST-GANG-SAME: {{^ }}gang
    //          PRT-OA-SAME: {{^ }}vector num_gangs(3){{$}}
    //          PRT-OA-NEXT: // for (k ={{.*}}) {
    //          PRT-OA-NEXT: //   {{printf|TGT_PRINTF}}
    //          PRT-OA-NEXT: //   for (l ={{.*}})
    //          PRT-OA-NEXT: //     ;
    //          PRT-OA-NEXT: // }
    //          PRT-OA-NEXT: // ^----------ACC----------^
    //
    // PRT-NOACC-NEXT: for (k ={{.*}}) {
    // PRT-NOACC-NEXT:   {{printf|TGT_PRINTF}}
    // PRT-NOACC-NEXT:   for (l ={{.*}})
    // PRT-NOACC-NEXT:     ;
    // PRT-NOACC-NEXT: }
    //
    // Gang-redundant mode executes each loop iteration per gang.
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    #pragma acc parallel loop auto GANG vector num_gangs(3)
    for (k = 1; k < 3; ++k) {
      TGT_PRINTF("loop iteration\n");
      for (l = 0; l < 2; ++l)
        ;
    }

    //. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 
    // acc parallel loop auto GANG worker vector
    //. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 

    // DMP-LABEL: StringLiteral {{.*}} "declared loop var: acc parallel loop auto GANG worker vector\n"
    // PRT-LABEL: printf("declared loop var: acc parallel loop auto GANG worker vector\n");
    //   EXE-NOT: {{.}}
    // EXE-LABEL: declared loop var: acc parallel loop auto GANG worker vector{{$}}
    //   EXE-NOT: {{.}}
    memset(&save, 0, sizeof(save));
    loopOnly = 88;
    loopOnlyArr[0] = 88;
    printf("declared loop var: acc parallel loop auto GANG worker vector\n");
    //           DMP: ACCParallelLoopDirective
    //      DMP-NEXT:   ACCNumGangsClause
    //      DMP-NEXT:     IntegerLiteral {{.*}} 'int' 3
    //      DMP-NEXT:   ACCAutoClause
    //       DMP-NOT:     <implicit>
    // DMP-GANG-NEXT:   ACCGangClause
    //  DMP-GANG-NOT:     <implicit>
    //      DMP-NEXT:   ACCWorkerClause
    //       DMP-NOT:     <implicit>
    //      DMP-NEXT:   ACCVectorClause
    //       DMP-NOT:     <implicit>
    //      DMP-NEXT:   effect: ACCParallelDirective
    //      DMP-NEXT:     ACCNumGangsClause
    //      DMP-NEXT:       IntegerLiteral {{.*}} 'int' 3
    //      DMP-NEXT:     ACCNomapClause {{.*}} <implicit>
    //      DMP-NEXT:       DeclRefExpr {{.*}} 'j' 'int'
    //      DMP-NEXT:       DeclRefExpr {{.*}} 'loopOnly' 'int'
    //      DMP-NEXT:     ACCCopyClause {{.*}} <implicit>
    //      DMP-NEXT:       DeclRefExpr {{.*}} 'save'
    //      DMP-NEXT:       DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    //      DMP-NEXT:     ACCSharedClause {{.*}} <implicit>
    //      DMP-NEXT:       DeclRefExpr {{.*}} 'save'
    //      DMP-NEXT:       DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    //      DMP-NEXT:     ACCFirstprivateClause {{.*}} <implicit>
    //      DMP-NEXT:       DeclRefExpr {{.*}} 'j' 'int'
    //      DMP-NEXT:       DeclRefExpr {{.*}} 'loopOnly' 'int'
    //      DMP-NEXT:     impl: OMPTargetTeamsDirective
    //      DMP-NEXT:       OMPNum_teamsClause
    //      DMP-NEXT:         IntegerLiteral {{.*}} 'int' 3
    //      DMP-NEXT:       OMPMapClause
    //       DMP-NOT:         <implicit>
    //      DMP-NEXT:         DeclRefExpr {{.*}} 'save'
    //      DMP-NEXT:         DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    //      DMP-NEXT:       OMPSharedClause
    //       DMP-NOT:         <implicit>
    //      DMP-NEXT:         DeclRefExpr {{.*}} 'save'
    //      DMP-NEXT:         DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    //      DMP-NEXT:       OMPFirstprivateClause
    //       DMP-NOT:         <implicit>
    //      DMP-NEXT:         DeclRefExpr {{.*}} 'j' 'int'
    //      DMP-NEXT:         DeclRefExpr {{.*}} 'loopOnly' 'int'
    //           DMP:     ACCLoopDirective
    //      DMP-NEXT:       ACCAutoClause
    //       DMP-NOT:         <implicit>
    // DMP-GANG-NEXT:       ACCGangClause
    //  DMP-GANG-NOT:         <implicit>
    //      DMP-NEXT:       ACCWorkerClause
    //       DMP-NOT:         <implicit>
    //      DMP-NEXT:       ACCVectorClause
    //       DMP-NOT:         <implicit>
    //      DMP-NEXT:       ACCSharedClause {{.*}} <implicit>
    //      DMP-NEXT:         DeclRefExpr {{.*}} 'j' 'int'
    //      DMP-NEXT:         DeclRefExpr {{.*}} 'save'
    //      DMP-NEXT:         DeclRefExpr {{.*}} 'loopOnly' 'int'
    //      DMP-NEXT:         DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    //      DMP-NEXT:       impl: ForStmt
    //
    //           PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(3) auto
    //       PRT-A-SRC-SAME: {{^ }}GANG
    //  PRT-A-AST-GANG-SAME: {{^ }}gang
    //           PRT-A-SAME: {{^ }}worker vector{{$}}
    //          PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(3) map(ompx_hold,tofrom: save,loopOnlyArr) shared(save,loopOnlyArr) firstprivate(j,loopOnly){{$}}
    //           PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(3) map(ompx_hold,tofrom: save,loopOnlyArr) shared(save,loopOnlyArr) firstprivate(j,loopOnly){{$}}
    //          PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(3) auto
    //      PRT-OA-SRC-SAME: {{^ }}GANG
    // PRT-OA-AST-GANG-SAME: {{^ }}gang
    //          PRT-OA-SAME: {{^ }}worker vector{{$}}
    //             PRT-NEXT: for ({{.*}}) {
    //                  PRT: }
    //
    // Gang-redundant mode executes each loop iteration per gang.
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    #pragma acc parallel loop num_gangs(3) auto GANG worker vector
    for (int i = 0; i < 2; ++i) {
      TGT_PRINTF("loop iteration\n");
      for (j = 0; j < 2; ++j)
        ;
      checkLoopSeqPrivate(save, i, loopOnly, 88, 77);
      loopOnly = 77;
      checkLoopSeqShared(save, i, loopOnlyArr, 88, 77);
      loopOnlyArr[0] = 77;
      // Each gang will see the final j from the j loop above.
      if (j != 2)
        save.declNestedLoopFoundBadVal = true;
    }
    // EXE-NEXT: save.loopOnlyAtI0FoundBadVal=0
    // EXE-NEXT: save.loopOnlyAtI1FoundBadVal=0
    // EXE-NEXT: save.loopOnlyArrFoundOldVal=1
    // EXE-NEXT: save.loopOnlyArrFoundBadVal=0
    // EXE-NEXT: save.declNestedLoopFoundBadVal=0
    // EXE-NEXT: loopOnly=88
    // EXE-NEXT: loopOnlyArr[0]=77
    printf("save.loopOnlyAtI0FoundBadVal=%d\n", save.loopOnlyAtI0FoundBadVal);
    printf("save.loopOnlyAtI1FoundBadVal=%d\n", save.loopOnlyAtI1FoundBadVal);
    printf("save.loopOnlyArrFoundOldVal=%d\n", save.loopOnlyArrFoundOldVal);
    printf("save.loopOnlyArrFoundBadVal=%d\n", save.loopOnlyArrFoundBadVal);
    printf("save.declNestedLoopFoundBadVal=%d\n", save.declNestedLoopFoundBadVal);
    printf("loopOnly=%d\n", loopOnly);
    printf("loopOnlyArr[0]=%d\n", loopOnlyArr[0]);

    // DMP-LABEL: StringLiteral {{.*}} "assigned loop var: acc parallel loop auto GANG worker vector\n"
    // PRT-LABEL: printf("assigned loop var: acc parallel loop auto GANG worker vector\n");
    //   EXE-NOT: {{.}}
    // EXE-LABEL: assigned loop var: acc parallel loop auto GANG worker vector{{$}}
    //   EXE-NOT: {{.}}
    printf("assigned loop var: acc parallel loop auto GANG worker vector\n");
    //           DMP: ACCParallelLoopDirective
    //      DMP-NEXT:   ACCAutoClause
    //       DMP-NOT:     <implicit>
    // DMP-GANG-NEXT:   ACCGangClause
    //  DMP-GANG-NOT:     <implicit>
    //      DMP-NEXT:   ACCWorkerClause
    //       DMP-NOT:     <implicit>
    //      DMP-NEXT:   ACCVectorClause
    //       DMP-NOT:     <implicit>
    //      DMP-NEXT:   ACCNumGangsClause
    //      DMP-NEXT:     IntegerLiteral {{.*}} 'int' 3
    //      DMP-NEXT:   effect: ACCParallelDirective
    //      DMP-NEXT:     ACCNumGangsClause
    //      DMP-NEXT:       IntegerLiteral {{.*}} 'int' 3
    //      DMP-NEXT:     ACCNomapClause {{.*}} <implicit>
    //      DMP-NEXT:       DeclRefExpr {{.*}} 'l' 'int'
    //      DMP-NEXT:     ACCFirstprivateClause {{.*}} <implicit>
    //      DMP-NEXT:       DeclRefExpr {{.*}} 'l' 'int'
    //      DMP-NEXT:     impl: OMPTargetTeamsDirective
    //      DMP-NEXT:       OMPNum_teamsClause
    //      DMP-NEXT:         IntegerLiteral {{.*}} 'int' 3
    //      DMP-NEXT:       OMPFirstprivateClause
    //       DMP-NOT:         <implicit>
    //      DMP-NEXT:         DeclRefExpr {{.*}} 'l' 'int'
    //           DMP:     ACCLoopDirective
    //      DMP-NEXT:       ACCAutoClause
    //       DMP-NOT:         <implicit>
    // DMP-GANG-NEXT:       ACCGangClause
    //  DMP-GANG-NOT:         <implicit>
    //      DMP-NEXT:       ACCWorkerClause
    //       DMP-NOT:         <implicit>
    //      DMP-NEXT:       ACCVectorClause
    //       DMP-NOT:         <implicit>
    //      DMP-NEXT:       ACCPrivateClause {{.*}} <predetermined>
    //      DMP-NEXT:         DeclRefExpr {{.*}} 'k' 'int'
    //      DMP-NEXT:       ACCSharedClause {{.*}} <implicit>
    //      DMP-NEXT:         DeclRefExpr {{.*}} 'l' 'int'
    //      DMP-NEXT:       impl: CompoundStmt
    //      DMP-NEXT:         DeclStmt
    //      DMP-NEXT:           VarDecl {{.*}} k 'int'
    //      DMP-NEXT:         ForStmt
    //
    //         PRT-AO-NEXT: // v----------ACC----------v
    //          PRT-A-NEXT: {{^ *}}#pragma acc parallel loop auto
    //      PRT-A-SRC-SAME: {{^ }}GANG
    // PRT-A-AST-GANG-SAME: {{^ }}gang
    //          PRT-A-SAME: {{^ }}worker vector num_gangs(3){{$}}
    //          PRT-A-NEXT: for (k ={{.*}}) {
    //          PRT-A-NEXT:   {{printf|TGT_PRINTF}}
    //          PRT-A-NEXT:   for (l ={{.*}})
    //          PRT-A-NEXT:     ;
    //          PRT-A-NEXT: }
    //         PRT-AO-NEXT: // ---------ACC->OMP--------
    //         PRT-AO-NEXT: // #pragma omp target teams num_teams(3) firstprivate(l){{$}}
    //         PRT-AO-NEXT: // {
    //         PRT-AO-NEXT: //   int k;
    //         PRT-AO-NEXT: //   for (k ={{.*}}) {
    //         PRT-AO-NEXT: //     {{printf|TGT_PRINTF}}
    //         PRT-AO-NEXT: //     for (l ={{.*}})
    //         PRT-AO-NEXT: //       ;
    //         PRT-AO-NEXT: //   }
    //         PRT-AO-NEXT: // }
    //         PRT-AO-NEXT: // ^----------OMP----------^
    //
    //          PRT-OA-NEXT: // v----------OMP----------v
    //           PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(3) firstprivate(l){{$}}
    //           PRT-O-NEXT: {
    //           PRT-O-NEXT:   int k;
    //           PRT-O-NEXT:   for (k ={{.*}}) {
    //           PRT-O-NEXT:     {{printf|TGT_PRINTF}}
    //           PRT-O-NEXT:     for (l ={{.*}})
    //           PRT-O-NEXT:       ;
    //           PRT-O-NEXT:   }
    //           PRT-O-NEXT: }
    //          PRT-OA-NEXT: // ---------OMP<-ACC--------
    //          PRT-OA-NEXT: // #pragma acc parallel loop auto
    //      PRT-OA-SRC-SAME: {{^ }}GANG
    // PRT-OA-AST-GANG-SAME: {{^ }}gang
    //          PRT-OA-SAME: {{^ }}worker vector num_gangs(3){{$}}
    //          PRT-OA-NEXT: // for (k ={{.*}}) {
    //          PRT-OA-NEXT: //   {{printf|TGT_PRINTF}}
    //          PRT-OA-NEXT: //   for (l ={{.*}})
    //          PRT-OA-NEXT: //     ;
    //          PRT-OA-NEXT: // }
    //          PRT-OA-NEXT: // ^----------ACC----------^
    //
    // PRT-NOACC-NEXT: for (k ={{.*}}) {
    // PRT-NOACC-NEXT:   {{printf|TGT_PRINTF}}
    // PRT-NOACC-NEXT:   for (l ={{.*}})
    // PRT-NOACC-NEXT:     ;
    // PRT-NOACC-NEXT: }
    //
    // Gang-redundant mode executes each loop iteration per gang.
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    #pragma acc parallel loop auto GANG worker vector num_gangs(3)
    for (k = 1; k < 3; ++k) {
      TGT_PRINTF("loop iteration\n");
      for (l = 0; l < 2; ++l)
        ;
    }
  }

  //............................................................................
  // acc parallel loop INDEPENDENT GANG
  //............................................................................

  {
    int loopOnly;
    int loopOnlyArr[1];
    int j = 0;
    struct {
      bool loopOnlyAtI0FoundBadVal;
      bool loopOnlyAtI1FoundBadVal;

      bool loopOnlyArrFoundOldVal;
      bool loopOnlyArrFoundBadVal;

      bool declNestedLoopOld;
      bool declNestedLoopNew;
      bool declNestedLoopErr;
    } save;

    // DMP-LABEL: StringLiteral {{.*}} "declared loop var: acc parallel loop INDEPENDENT GANG\n"
    // PRT-LABEL: printf("declared loop var: acc parallel loop INDEPENDENT GANG\n");
    //   EXE-NOT: {{.}}
    // EXE-LABEL: declared loop var: acc parallel loop INDEPENDENT GANG{{$}}
    //   EXE-NOT: {{.}}
    memset(&save, 0, sizeof(save));
    loopOnly = 88;
    loopOnlyArr[0] = 88;
    printf("declared loop var: acc parallel loop INDEPENDENT GANG\n");

    //              DMP: ACCParallelLoopDirective
    //         DMP-NEXT:   ACCNumGangsClause
    //         DMP-NEXT:     IntegerLiteral {{.*}} 'int' 3
    //     DMP-IND-NEXT:   ACCIndependentClause
    //      DMP-IND-NOT:     <implicit>
    //    DMP-GANG-NEXT:   ACCGangClause
    //     DMP-GANG-NOT:     <implicit>
    //         DMP-NEXT:   effect: ACCParallelDirective
    //         DMP-NEXT:     ACCNumGangsClause
    //         DMP-NEXT:       IntegerLiteral {{.*}} 'int' 3
    //         DMP-NEXT:     ACCNomapClause {{.*}} <implicit>
    //         DMP-NEXT:       DeclRefExpr {{.*}} 'j' 'int'
    //         DMP-NEXT:       DeclRefExpr {{.*}} 'loopOnly' 'int'
    //         DMP-NEXT:     ACCCopyClause {{.*}} <implicit>
    //         DMP-NEXT:       DeclRefExpr {{.*}} 'save'
    //         DMP-NEXT:       DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    //         DMP-NEXT:     ACCSharedClause {{.*}} <implicit>
    //         DMP-NEXT:       DeclRefExpr {{.*}} 'save'
    //         DMP-NEXT:       DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    //         DMP-NEXT:     ACCFirstprivateClause {{.*}} <implicit>
    //         DMP-NEXT:       DeclRefExpr {{.*}} 'j' 'int'
    //         DMP-NEXT:       DeclRefExpr {{.*}} 'loopOnly' 'int'
    //         DMP-NEXT:     impl: OMPTargetTeamsDirective
    //         DMP-NEXT:       OMPNum_teamsClause
    //         DMP-NEXT:         IntegerLiteral {{.*}} 'int' 3
    //         DMP-NEXT:       OMPMapClause
    //          DMP-NOT:         <implicit>
    //         DMP-NEXT:         DeclRefExpr {{.*}} 'save'
    //         DMP-NEXT:         DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    //         DMP-NEXT:       OMPSharedClause
    //          DMP-NOT:         <implicit>
    //         DMP-NEXT:         DeclRefExpr {{.*}} 'save'
    //         DMP-NEXT:         DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    //         DMP-NEXT:       OMPFirstprivateClause
    //         DMP-NOT:          <implicit>
    //         DMP-NEXT:         DeclRefExpr {{.*}} 'j' 'int'
    //         DMP-NEXT:         DeclRefExpr {{.*}} 'loopOnly' 'int'
    //              DMP:     ACCLoopDirective
    //     DMP-IND-NEXT:       ACCIndependentClause
    //      DMP-IND-NOT:         <implicit>
    //    DMP-GANG-NEXT:       ACCGangClause
    //     DMP-GANG-NOT:         <implicit>
    //  DMP-NO-IND-NEXT:       ACCIndependentClause {{.*}} <implicit>
    //         DMP-NEXT:       ACCSharedClause {{.*}} <implicit>
    //         DMP-NEXT:         DeclRefExpr {{.*}} 'j' 'int'
    //         DMP-NEXT:         DeclRefExpr {{.*}} 'save'
    //         DMP-NEXT:         DeclRefExpr {{.*}} 'loopOnly' 'int'
    //         DMP-NEXT:         DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    // DMP-NO-GANG-NEXT:       ACCGangClause {{.*}} <implicit>
    //         DMP-NEXT:       impl: OMPDistributeDirective
    //         DMP-NEXT:         OMPSharedClause {{.*}} <implicit>
    //         DMP-NEXT:           DeclRefExpr {{.*}} 'j' 'int'
    //         DMP-NEXT:           DeclRefExpr {{.*}} 'save'
    //         DMP-NEXT:           DeclRefExpr {{.*}} 'loopOnly' 'int'
    //         DMP-NEXT:           DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    //              DMP:         ForStmt
    //
    //           PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(3)
    //       PRT-A-SRC-SAME: {{^ }}INDEPENDENT GANG
    //   PRT-A-AST-IND-SAME: {{^ }}independent
    //  PRT-A-AST-GANG-SAME: {{^ }}gang
    //           PRT-A-SAME: {{^$}}
    //          PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(3) map(ompx_hold,tofrom: save,loopOnlyArr) shared(save,loopOnlyArr) firstprivate(j,loopOnly){{$}}
    //          PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
    //           PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(3) map(ompx_hold,tofrom: save,loopOnlyArr) shared(save,loopOnlyArr) firstprivate(j,loopOnly){{$}}
    //           PRT-O-NEXT: {{^ *}}#pragma omp distribute{{$}}
    //          PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(3)
    //      PRT-OA-SRC-SAME: {{^ }}INDEPENDENT GANG
    //  PRT-OA-AST-IND-SAME: {{^ }}independent
    // PRT-OA-AST-GANG-SAME: {{^ }}gang
    //          PRT-OA-SAME: {{^$}}
    //             PRT-NEXT: for ({{.*}}) {
    //                  PRT: }
    //
    // Gang-partitioned mode executes each loop iteration once.
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    #pragma acc parallel loop num_gangs(3) INDEPENDENT GANG
    for (int i = 0; i < 2; ++i) {
      TGT_PRINTF("loop iteration\n");
      for (j = 0; j < 2; ++j)
        ;
      checkLoopPartPrivate(save, i, loopOnly, 88, 77);
      loopOnly = 77;
      checkLoopPartShared(save, i, loopOnlyArr, loopOnlyArr[0], 88, 77);
      loopOnlyArr[0] = 77;
      // Each gang that is assigned one or more i iterations will see the final
      // j from the j loop above, and the remaining gangs will not.
      if (j == 0)
        save.declNestedLoopOld = true;
      else if (j == 2)
        save.declNestedLoopNew = true;
      else
        save.declNestedLoopErr = true;
    }

    // EXE-NEXT: save.loopOnlyAtI0FoundBadVal=0
    // EXE-NEXT: save.loopOnlyAtI1FoundBadVal=0
    // EXE-NEXT: save.loopOnlyArrFoundOldVal=1
    // EXE-NEXT: save.loopOnlyArrFoundBadVal=0
    // EXE-NEXT: save.declNestedLoopOld=0
    // EXE-NEXT: save.declNestedLoopNew=1
    // EXE-NEXT: save.declNestedLoopErr=0
    // EXE-NEXT: loopOnly=88
    // EXE-NEXT: loopOnlyArr[0]=77
    printf("save.loopOnlyAtI0FoundBadVal=%d\n", save.loopOnlyAtI0FoundBadVal);
    printf("save.loopOnlyAtI1FoundBadVal=%d\n", save.loopOnlyAtI1FoundBadVal);
    printf("save.loopOnlyArrFoundOldVal=%d\n", save.loopOnlyArrFoundOldVal);
    printf("save.loopOnlyArrFoundBadVal=%d\n", save.loopOnlyArrFoundBadVal);
    printf("save.declNestedLoopOld=%d\n", save.declNestedLoopOld);
    printf("save.declNestedLoopNew=%d\n", save.declNestedLoopNew);
    printf("save.declNestedLoopErr=%d\n", save.declNestedLoopErr);
    printf("loopOnly=%d\n", loopOnly);
    printf("loopOnlyArr[0]=%d\n", loopOnlyArr[0]);
  }

  {
    // DMP-LABEL: StringLiteral {{.*}} "assigned loop var: acc parallel loop INDEPENDENT GANG\n"
    // PRT-LABEL: printf("assigned loop var: acc parallel loop INDEPENDENT GANG\n");
    //   EXE-NOT: {{.}}
    // EXE-LABEL: assigned loop var: acc parallel loop INDEPENDENT GANG{{$}}
    //   EXE-NOT: {{.}}
    int k, l;
    printf("assigned loop var: acc parallel loop INDEPENDENT GANG\n");
    //              DMP: ACCParallelLoopDirective
    //         DMP-NEXT:   ACCNumGangsClause
    //         DMP-NEXT:     IntegerLiteral {{.*}} 'int' 3
    //     DMP-IND-NEXT:   ACCIndependentClause
    //      DMP-IND-NOT:     <implicit>
    //    DMP-GANG-NEXT:   ACCGangClause
    //     DMP-GANG-NOT:     <implicit>
    //         DMP-NEXT:   effect: ACCParallelDirective
    //         DMP-NEXT:     ACCNumGangsClause
    //         DMP-NEXT:       IntegerLiteral {{.*}} 'int' 3
    //         DMP-NEXT:     ACCNomapClause {{.*}} <implicit>
    //         DMP-NEXT:       DeclRefExpr {{.*}} 'l' 'int'
    //         DMP-NEXT:     ACCFirstprivateClause {{.*}} <implicit>
    //         DMP-NEXT:       DeclRefExpr {{.*}} 'l' 'int'
    //         DMP-NEXT:     impl: OMPTargetTeamsDirective
    //         DMP-NEXT:       OMPNum_teamsClause
    //         DMP-NEXT:         IntegerLiteral {{.*}} 'int' 3
    //         DMP-NEXT:       OMPFirstprivateClause
    //          DMP-NOT:         <implicit>
    //         DMP-NEXT:         DeclRefExpr {{.*}} 'l' 'int'
    //              DMP:     ACCLoopDirective
    //     DMP-IND-NEXT:       ACCIndependentClause
    //      DMP-IND-NOT:         <implicit>
    //    DMP-GANG-NEXT:       ACCGangClause
    //     DMP-GANG-NOT:         <implicit>
    //  DMP-NO-IND-NEXT:       ACCIndependentClause {{.*}} <implicit>
    //         DMP-NEXT:       ACCPrivateClause {{.*}} <predetermined>
    //         DMP-NEXT:         DeclRefExpr {{.*}} 'k' 'int'
    //         DMP-NEXT:       ACCSharedClause {{.*}} <implicit>
    //         DMP-NEXT:         DeclRefExpr {{.*}} 'l' 'int'
    // DMP-NO-GANG-NEXT:       ACCGangClause {{.*}} <implicit>
    //         DMP-NEXT:       impl: OMPDistributeDirective
    //         DMP-NEXT:         OMPPrivateClause
    //          DMP-NOT:           <implicit>
    //         DMP-NEXT:           DeclRefExpr {{.*}} 'k' 'int'
    //         DMP-NEXT:         OMPSharedClause {{.*}} <implicit>
    //         DMP-NEXT:           DeclRefExpr {{.*}} 'l' 'int'
    //              DMP:         ForStmt
    //
    //           PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(3)
    //       PRT-A-SRC-SAME: {{^ }}INDEPENDENT GANG
    //   PRT-A-AST-IND-SAME: {{^ }}independent
    //  PRT-A-AST-GANG-SAME: {{^ }}gang
    //           PRT-A-SAME: {{^$}}
    //          PRT-AO-NEXT: // #pragma omp target teams num_teams(3) firstprivate(l){{$}}
    //          PRT-AO-NEXT: // #pragma omp distribute private(k){{$}}
    //           PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(3) firstprivate(l){{$}}
    //           PRT-O-NEXT: {{^ *}}#pragma omp distribute private(k){{$}}
    //          PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(3)
    //      PRT-OA-SRC-SAME: {{^ }}INDEPENDENT GANG
    //  PRT-OA-AST-IND-SAME: {{^ }}independent
    // PRT-OA-AST-GANG-SAME: {{^ }}gang
    //          PRT-OA-SAME: {{^$}}
    //             PRT-NEXT: for (k ={{.*}}) {
    //             PRT-NEXT:   {{printf|TGT_PRINTF}}
    //             PRT-NEXT:   for (l ={{.*}})
    //             PRT-NEXT:     ;
    //             PRT-NEXT: }
    //
    // Gang-partitioned mode executes each loop iteration once.
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    #pragma acc parallel loop num_gangs(3) INDEPENDENT GANG
    for (k = 1; k < 3; ++k) {
      TGT_PRINTF("loop iteration\n");
      for (l = 0; l < 2; ++l)
        ;
    }
  }

  //............................................................................
  // acc parallel loop INDEPENDENT GANG worker
  //............................................................................

  {
    int loopOnly;
    int loopOnlyArr[1];
    struct {
      bool loopOnlyFoundOldVal;
      bool loopOnlyFoundBadVal;

      bool loopOnlyArrFoundOldVal;
      bool loopOnlyArrFoundBadVal;
    } save;

    // DMP-LABEL: StringLiteral {{.*}} "declared loop var: acc parallel loop INDEPENDENT GANG worker\n"
    // PRT-LABEL: printf("declared loop var: acc parallel loop INDEPENDENT GANG worker\n");
    //   EXE-NOT: {{.}}
    // EXE-LABEL: declared loop var: acc parallel loop INDEPENDENT GANG worker{{$}}
    //   EXE-NOT: {{.}}
    memset(&save, 0, sizeof(save));
    loopOnly = 88;
    loopOnlyArr[0] = 88;
    printf("declared loop var: acc parallel loop INDEPENDENT GANG worker\n");

// FIXME: amdgcn misbehaves sometimes for worker loops.
// PRT-SRC-NEXT: #if !TGT_AMDGCN
#if !TGT_AMDGCN
    //              DMP: ACCParallelLoopDirective
    //         DMP-NEXT:   ACCNumGangsClause
    //         DMP-NEXT:     IntegerLiteral {{.*}} 'int' 3
    //     DMP-IND-NEXT:   ACCIndependentClause
    //      DMP-IND-NOT:     <implicit>
    //    DMP-GANG-NEXT:   ACCGangClause
    //     DMP-GANG-NOT:     <implicit>
    //         DMP-NEXT:   ACCWorkerClause
    //          DMP-NOT:     <implicit>
    //         DMP-NEXT:   effect: ACCParallelDirective
    //         DMP-NEXT:     ACCNumGangsClause
    //         DMP-NEXT:       IntegerLiteral {{.*}} 'int' 3
    //         DMP-NEXT:     ACCNomapClause {{.*}} <implicit>
    //         DMP-NEXT:       DeclRefExpr {{.*}} 'loopOnly' 'int'
    //         DMP-NEXT:     ACCCopyClause {{.*}} <implicit>
    //         DMP-NEXT:       DeclRefExpr {{.*}} 'save'
    //         DMP-NEXT:       DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    //         DMP-NEXT:     ACCSharedClause {{.*}} <implicit>
    //         DMP-NEXT:       DeclRefExpr {{.*}} 'save'
    //         DMP-NEXT:       DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    //         DMP-NEXT:     ACCFirstprivateClause {{.*}} <implicit>
    //         DMP-NEXT:       DeclRefExpr {{.*}} 'loopOnly' 'int'
    //         DMP-NEXT:     impl: OMPTargetTeamsDirective
    //         DMP-NEXT:       OMPNum_teamsClause
    //         DMP-NEXT:         IntegerLiteral {{.*}} 'int' 3
    //         DMP-NEXT:       OMPMapClause
    //          DMP-NOT:         <implicit>
    //         DMP-NEXT:         DeclRefExpr {{.*}} 'save'
    //         DMP-NEXT:         DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    //         DMP-NEXT:       OMPSharedClause
    //          DMP-NOT:         <implicit>
    //         DMP-NEXT:         DeclRefExpr {{.*}} 'save'
    //         DMP-NEXT:         DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    //         DMP-NEXT:       OMPFirstprivateClause
    //         DMP-NOT:          <implicit>
    //         DMP-NEXT:         DeclRefExpr {{.*}} 'loopOnly' 'int'
    //              DMP:     ACCLoopDirective
    //     DMP-IND-NEXT:       ACCIndependentClause
    //      DMP-IND-NOT:         <implicit>
    //    DMP-GANG-NEXT:       ACCGangClause
    //     DMP-GANG-NOT:         <implicit>
    //         DMP-NEXT:       ACCWorkerClause
    //          DMP-NOT:         <implicit>
    //  DMP-NO-IND-NEXT:       ACCIndependentClause {{.*}} <implicit>
    //         DMP-NEXT:       ACCSharedClause {{.*}} <implicit>
    //         DMP-NEXT:         DeclRefExpr {{.*}} 'save'
    //         DMP-NEXT:         DeclRefExpr {{.*}} 'loopOnly' 'int'
    //         DMP-NEXT:         DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    // DMP-NO-GANG-NEXT:       ACCGangClause {{.*}} <implicit>
    //         DMP-NEXT:       impl: OMPDistributeParallelForDirective
    //         DMP-NEXT:         OMPSharedClause
    //          DMP-NOT:           <implicit>
    //         DMP-NEXT:           DeclRefExpr {{.*}} 'save'
    //         DMP-NEXT:           DeclRefExpr {{.*}} 'loopOnly' 'int'
    //         DMP-NEXT:           DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    //              DMP:         ForStmt
    //
    //           PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(3)
    //       PRT-A-SRC-SAME: {{^ }}INDEPENDENT GANG
    //   PRT-A-AST-IND-SAME: {{^ }}independent
    //  PRT-A-AST-GANG-SAME: {{^ }}gang
    //           PRT-A-SAME: {{^ }}worker{{$}}
    //          PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(3) map(ompx_hold,tofrom: save,loopOnlyArr) shared(save,loopOnlyArr) firstprivate(loopOnly){{$}}
    //          PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for shared(save,loopOnly,loopOnlyArr){{$}}
    //           PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(3) map(ompx_hold,tofrom: save,loopOnlyArr) shared(save,loopOnlyArr) firstprivate(loopOnly){{$}}
    //           PRT-O-NEXT: {{^ *}}#pragma omp distribute parallel for shared(save,loopOnly,loopOnlyArr){{$}}
    //          PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(3)
    //      PRT-OA-SRC-SAME: {{^ }}INDEPENDENT GANG
    //  PRT-OA-AST-IND-SAME: {{^ }}independent
    // PRT-OA-AST-GANG-SAME: {{^ }}gang
    //          PRT-OA-SAME: {{^ }}worker{{$}}
    //             PRT-NEXT: for ({{.*}}) {
    //                  PRT: }
    //
    // Gang-partitioned mode executes each loop iteration once.
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    #pragma acc parallel loop num_gangs(3) INDEPENDENT GANG worker
    for (int i = 0; i < 2; ++i) {
      TGT_PRINTF("loop iteration\n");
      // We omit checking of an inner j loop this time.  j would be shared
      // among workers and thus would have an unpredictable value.
      checkLoopPartShared(save, i, loopOnly, loopOnly, 88, 77);
      loopOnly = 77;
      checkLoopPartShared(save, i, loopOnlyArr, loopOnlyArr[0], 88, 77);
      loopOnlyArr[0] = 77;
    }
#endif

    // EXE-NEXT: save.loopOnlyFoundOldVal=1
    // EXE-NEXT: save.loopOnlyFoundBadVal=0
    // EXE-NEXT: save.loopOnlyArrFoundOldVal=1
    // EXE-NEXT: save.loopOnlyArrFoundBadVal=0
    // EXE-NEXT: loopOnly=88
    // EXE-NEXT: loopOnlyArr[0]=77
// FIXME: amdgcn misbehaves sometimes for worker loops.
#if !TGT_AMDGCN
    printf("save.loopOnlyFoundOldVal=%d\n", save.loopOnlyFoundOldVal);
    printf("save.loopOnlyFoundBadVal=%d\n", save.loopOnlyFoundBadVal);
    printf("save.loopOnlyArrFoundOldVal=%d\n", save.loopOnlyArrFoundOldVal);
    printf("save.loopOnlyArrFoundBadVal=%d\n", save.loopOnlyArrFoundBadVal);
    printf("loopOnly=%d\n", loopOnly);
    printf("loopOnlyArr[0]=%d\n", loopOnlyArr[0]);
#else
    printf("save.loopOnlyFoundOldVal=%d\n", 1);
    printf("save.loopOnlyFoundBadVal=%d\n", 0);
    printf("save.loopOnlyArrFoundOldVal=%d\n", 1);
    printf("save.loopOnlyArrFoundBadVal=%d\n", 0);
    printf("loopOnly=%d\n", 88);
    printf("loopOnlyArr[0]=%d\n", 77);
#endif
  }

  {
    // DMP-LABEL: StringLiteral {{.*}} "assigned loop var: acc parallel loop INDEPENDENT GANG worker\n"
    // PRT-LABEL: printf("assigned loop var: acc parallel loop INDEPENDENT GANG worker\n");
    //   EXE-NOT: {{.}}
    // EXE-LABEL: assigned loop var: acc parallel loop INDEPENDENT GANG worker{{$}}
    //   EXE-NOT: {{.}}
    int k, l;
    printf("assigned loop var: acc parallel loop INDEPENDENT GANG worker\n");

// FIXME: amdgcn misbehaves sometimes for worker loops.
// PRT-SRC-NEXT: #if !TGT_AMDGCN
#if !TGT_AMDGCN
    //              DMP: ACCParallelLoopDirective
    //         DMP-NEXT:   ACCNumGangsClause
    //         DMP-NEXT:     IntegerLiteral {{.*}} 'int' 3
    //     DMP-IND-NEXT:   ACCIndependentClause
    //      DMP-IND-NOT:     <implicit>
    //    DMP-GANG-NEXT:   ACCGangClause
    //     DMP-GANG-NOT:     <implicit>
    //         DMP-NEXT:   ACCWorkerClause
    //          DMP-NOT:     <implicit>
    //         DMP-NEXT:   effect: ACCParallelDirective
    //         DMP-NEXT:     ACCNumGangsClause
    //         DMP-NEXT:       IntegerLiteral {{.*}} 'int' 3
    //         DMP-NEXT:     ACCNomapClause {{.*}} <implicit>
    //         DMP-NEXT:       DeclRefExpr {{.*}} 'l' 'int'
    //         DMP-NEXT:     ACCFirstprivateClause {{.*}} <implicit>
    //         DMP-NEXT:       DeclRefExpr {{.*}} 'l' 'int'
    //         DMP-NEXT:     impl: OMPTargetTeamsDirective
    //         DMP-NEXT:       OMPNum_teamsClause
    //         DMP-NEXT:         IntegerLiteral {{.*}} 'int' 3
    //         DMP-NEXT:       OMPFirstprivateClause
    //          DMP-NOT:         <implicit>
    //         DMP-NEXT:         DeclRefExpr {{.*}} 'l' 'int'
    //              DMP:     ACCLoopDirective
    //     DMP-IND-NEXT:       ACCIndependentClause
    //      DMP-IND-NOT:         <implicit>
    //    DMP-GANG-NEXT:       ACCGangClause
    //     DMP-GANG-NOT:         <implicit>
    //         DMP-NEXT:       ACCWorkerClause
    //          DMP-NOT:         <implicit>
    //  DMP-NO-IND-NEXT:       ACCIndependentClause {{.*}} <implicit>
    //         DMP-NEXT:       ACCPrivateClause {{.*}} <predetermined>
    //         DMP-NEXT:         DeclRefExpr {{.*}} 'k' 'int'
    //         DMP-NEXT:       ACCSharedClause {{.*}} <implicit>
    //         DMP-NEXT:         DeclRefExpr {{.*}} 'l' 'int'
    // DMP-NO-GANG-NEXT:       ACCGangClause {{.*}} <implicit>
    //         DMP-NEXT:       impl: OMPDistributeParallelForDirective
    //         DMP-NEXT:         OMPPrivateClause
    //          DMP-NOT:           <implicit>
    //         DMP-NEXT:           DeclRefExpr {{.*}} 'k' 'int'
    //         DMP-NEXT:         OMPSharedClause
    //          DMP-NOT:           <implicit>
    //         DMP-NEXT:           DeclRefExpr {{.*}} 'l' 'int'
    //              DMP:         ForStmt
    //
    //           PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(3)
    //       PRT-A-SRC-SAME: {{^ }}INDEPENDENT GANG
    //   PRT-A-AST-IND-SAME: {{^ }}independent
    //  PRT-A-AST-GANG-SAME: {{^ }}gang
    //           PRT-A-SAME: {{^ }}worker{{$}}
    //          PRT-AO-NEXT: // #pragma omp target teams num_teams(3) firstprivate(l){{$}}
    //          PRT-AO-NEXT: // #pragma omp distribute parallel for private(k) shared(l){{$}}
    //           PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(3) firstprivate(l){{$}}
    //           PRT-O-NEXT: {{^ *}}#pragma omp distribute parallel for private(k) shared(l){{$}}
    //          PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(3)
    //      PRT-OA-SRC-SAME: {{^ }}INDEPENDENT GANG
    //  PRT-OA-AST-IND-SAME: {{^ }}independent
    // PRT-OA-AST-GANG-SAME: {{^ }}gang
    //          PRT-OA-SAME: {{^ }}worker{{$}}
    //             PRT-NEXT: for (k ={{.*}}) {
    //             PRT-NEXT:   {{printf|TGT_PRINTF}}
    //             PRT-NEXT:   for (l ={{.*}})
    //             PRT-NEXT:     ;
    //             PRT-NEXT: }
    //
    // Gang-partitioned mode executes each loop iteration once.
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    #pragma acc parallel loop num_gangs(3) INDEPENDENT GANG worker
    for (k = 1; k < 3; ++k) {
      TGT_PRINTF("loop iteration\n");
      for (l = 0; l < 2; ++l)
        ;
    }
#endif
  }

  //............................................................................
  // acc parallel loop INDEPENDENT GANG vector
  //............................................................................

  {
    int loopOnly;
    int loopOnlyArr[1];
    struct {
      bool loopOnlyFoundOldVal;
      bool loopOnlyFoundBadVal;

      bool loopOnlyArrFoundOldVal;
      bool loopOnlyArrFoundBadVal;
    } save;

    // DMP-LABEL: StringLiteral {{.*}} "declared loop var: acc parallel loop INDEPENDENT GANG vector\n"
    // PRT-LABEL: printf("declared loop var: acc parallel loop INDEPENDENT GANG vector\n");
    //   EXE-NOT: {{.}}
    // EXE-LABEL: declared loop var: acc parallel loop INDEPENDENT GANG vector{{$}}
    //   EXE-NOT: {{.}}
    memset(&save, 0, sizeof(save));
    loopOnly = 88;
    loopOnlyArr[0] = 88;
    printf("declared loop var: acc parallel loop INDEPENDENT GANG vector\n");

    //              DMP: ACCParallelLoopDirective
    //         DMP-NEXT:   ACCNumGangsClause
    //         DMP-NEXT:     IntegerLiteral {{.*}} 'int' 3
    //     DMP-IND-NEXT:   ACCIndependentClause
    //      DMP-IND-NOT:     <implicit>
    //    DMP-GANG-NEXT:   ACCGangClause
    //     DMP-GANG-NOT:     <implicit>
    //         DMP-NEXT:   ACCVectorClause
    //          DMP-NOT:     <implicit>
    //         DMP-NEXT:   effect: ACCParallelDirective
    //         DMP-NEXT:     ACCNumGangsClause
    //         DMP-NEXT:       IntegerLiteral {{.*}} 'int' 3
    //         DMP-NEXT:     ACCNomapClause {{.*}} <implicit>
    //         DMP-NEXT:       DeclRefExpr {{.*}} 'loopOnly' 'int'
    //         DMP-NEXT:     ACCCopyClause {{.*}} <implicit>
    //         DMP-NEXT:       DeclRefExpr {{.*}} 'save'
    //         DMP-NEXT:       DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    //         DMP-NEXT:     ACCSharedClause {{.*}} <implicit>
    //         DMP-NEXT:       DeclRefExpr {{.*}} 'save'
    //         DMP-NEXT:       DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    //         DMP-NEXT:     ACCFirstprivateClause {{.*}} <implicit>
    //         DMP-NEXT:       DeclRefExpr {{.*}} 'loopOnly' 'int'
    //         DMP-NEXT:     impl: OMPTargetTeamsDirective
    //         DMP-NEXT:       OMPNum_teamsClause
    //         DMP-NEXT:         IntegerLiteral {{.*}} 'int' 3
    //         DMP-NEXT:       OMPMapClause
    //          DMP-NOT:         <implicit>
    //         DMP-NEXT:         DeclRefExpr {{.*}} 'save'
    //         DMP-NEXT:         DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    //         DMP-NEXT:       OMPSharedClause
    //          DMP-NOT:         <implicit>
    //         DMP-NEXT:         DeclRefExpr {{.*}} 'save'
    //         DMP-NEXT:         DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    //         DMP-NEXT:       OMPFirstprivateClause
    //         DMP-NOT:          <implicit>
    //         DMP-NEXT:         DeclRefExpr {{.*}} 'loopOnly' 'int'
    //              DMP:     ACCLoopDirective
    //     DMP-IND-NEXT:       ACCIndependentClause
    //      DMP-IND-NOT:         <implicit>
    //    DMP-GANG-NEXT:       ACCGangClause
    //     DMP-GANG-NOT:         <implicit>
    //         DMP-NEXT:       ACCVectorClause
    //          DMP-NOT:         <implicit>
    //  DMP-NO-IND-NEXT:       ACCIndependentClause {{.*}} <implicit>
    //         DMP-NEXT:       ACCSharedClause {{.*}} <implicit>
    //         DMP-NEXT:         DeclRefExpr {{.*}} 'save'
    //         DMP-NEXT:         DeclRefExpr {{.*}} 'loopOnly' 'int'
    //         DMP-NEXT:         DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    // DMP-NO-GANG-NEXT:       ACCGangClause {{.*}} <implicit>
    //         DMP-NEXT:       impl: OMPDistributeSimdDirective
    //         DMP-NEXT:         OMPSharedClause {{.*}} <implicit>
    //         DMP-NEXT:           DeclRefExpr {{.*}} 'save'
    //         DMP-NEXT:           DeclRefExpr {{.*}} 'loopOnly' 'int'
    //         DMP-NEXT:           DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    //              DMP:         ForStmt
    //
    //           PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(3)
    //       PRT-A-SRC-SAME: {{^ }}INDEPENDENT GANG
    //   PRT-A-AST-IND-SAME: {{^ }}independent
    //  PRT-A-AST-GANG-SAME: {{^ }}gang
    //           PRT-A-SAME: {{^ }}vector{{$}}
    //          PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(3) map(ompx_hold,tofrom: save,loopOnlyArr) shared(save,loopOnlyArr) firstprivate(loopOnly){{$}}
    //          PRT-AO-NEXT: {{^ *}}// #pragma omp distribute simd{{$}}
    //           PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(3) map(ompx_hold,tofrom: save,loopOnlyArr) shared(save,loopOnlyArr) firstprivate(loopOnly){{$}}
    //           PRT-O-NEXT: {{^ *}}#pragma omp distribute simd{{$}}
    //          PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(3)
    //      PRT-OA-SRC-SAME: {{^ }}INDEPENDENT GANG
    //  PRT-OA-AST-IND-SAME: {{^ }}independent
    // PRT-OA-AST-GANG-SAME: {{^ }}gang
    //          PRT-OA-SAME: {{^ }}vector{{$}}
    //             PRT-NEXT: for ({{.*}}) {
    //                  PRT: }
    //
    // Gang-partitioned mode executes each loop iteration once.
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    #pragma acc parallel loop num_gangs(3) INDEPENDENT GANG vector
    for (int i = 0; i < 2; ++i) {
      TGT_PRINTF("loop iteration\n");
      // We omit checking of an inner j loop this time.  j would be shared
      // among vector lanes and thus would have an unpredictable value.
      checkLoopPartShared(save, i, loopOnly, loopOnly, 88, 77);
      loopOnly = 77;
      checkLoopPartShared(save, i, loopOnlyArr, loopOnlyArr[0], 88, 77);
      loopOnlyArr[0] = 77;
    }

    // EXE-NEXT: save.loopOnlyFoundOldVal=1
    // EXE-NEXT: save.loopOnlyFoundBadVal=0
    // EXE-NEXT: save.loopOnlyArrFoundOldVal=1
    // EXE-NEXT: save.loopOnlyArrFoundBadVal=0
    // EXE-NEXT: loopOnly=88
    // EXE-NEXT: loopOnlyArr[0]=77
    printf("save.loopOnlyFoundOldVal=%d\n", save.loopOnlyFoundOldVal);
    printf("save.loopOnlyFoundBadVal=%d\n", save.loopOnlyFoundBadVal);
    printf("save.loopOnlyArrFoundOldVal=%d\n", save.loopOnlyArrFoundOldVal);
    printf("save.loopOnlyArrFoundBadVal=%d\n", save.loopOnlyArrFoundBadVal);
    printf("loopOnly=%d\n", loopOnly);
    printf("loopOnlyArr[0]=%d\n", loopOnlyArr[0]);
  }

  {
    // DMP-LABEL: StringLiteral {{.*}} "assigned loop var: acc parallel loop INDEPENDENT GANG vector\n"
    // PRT-LABEL: printf("assigned loop var: acc parallel loop INDEPENDENT GANG vector\n");
    //   EXE-NOT: {{.}}
    // EXE-LABEL: assigned loop var: acc parallel loop INDEPENDENT GANG vector{{$}}
    //   EXE-NOT: {{.}}
    int k, l;
    printf("assigned loop var: acc parallel loop INDEPENDENT GANG vector\n");
    //              DMP: ACCParallelLoopDirective
    //         DMP-NEXT:   ACCNumGangsClause
    //         DMP-NEXT:     IntegerLiteral {{.*}} 'int' 3
    //     DMP-IND-NEXT:   ACCIndependentClause
    //      DMP-IND-NOT:     <implicit>
    //    DMP-GANG-NEXT:   ACCGangClause
    //     DMP-GANG-NOT:     <implicit>
    //         DMP-NEXT:   ACCVectorClause
    //          DMP-NOT:     <implicit>
    //         DMP-NEXT:   effect: ACCParallelDirective
    //         DMP-NEXT:     ACCNumGangsClause
    //         DMP-NEXT:       IntegerLiteral {{.*}} 'int' 3
    //         DMP-NEXT:     ACCNomapClause {{.*}} <implicit>
    //         DMP-NEXT:       DeclRefExpr {{.*}} 'l' 'int'
    //         DMP-NEXT:     ACCFirstprivateClause {{.*}} <implicit>
    //         DMP-NEXT:       DeclRefExpr {{.*}} 'l' 'int'
    //         DMP-NEXT:     impl: OMPTargetTeamsDirective
    //         DMP-NEXT:       OMPNum_teamsClause
    //         DMP-NEXT:         IntegerLiteral {{.*}} 'int' 3
    //         DMP-NEXT:       OMPFirstprivateClause
    //          DMP-NOT:         <implicit>
    //         DMP-NEXT:         DeclRefExpr {{.*}} 'l' 'int'
    //              DMP:     ACCLoopDirective
    //     DMP-IND-NEXT:       ACCIndependentClause
    //      DMP-IND-NOT:         <implicit>
    //    DMP-GANG-NEXT:       ACCGangClause
    //     DMP-GANG-NOT:         <implicit>
    //         DMP-NEXT:       ACCVectorClause
    //          DMP-NOT:         <implicit>
    //  DMP-NO-IND-NEXT:       ACCIndependentClause {{.*}} <implicit>
    //         DMP-NEXT:       ACCPrivateClause {{.*}} <predetermined>
    //         DMP-NEXT:         DeclRefExpr {{.*}} 'k' 'int'
    //         DMP-NEXT:       ACCSharedClause {{.*}} <implicit>
    //         DMP-NEXT:         DeclRefExpr {{.*}} 'l' 'int'
    // DMP-NO-GANG-NEXT:       ACCGangClause {{.*}} <implicit>
    //         DMP-NEXT:       impl: CompoundStmt
    //         DMP-NEXT:         DeclStmt
    //         DMP-NEXT:           VarDecl {{.*}} k 'int'
    //         DMP-NEXT:         OMPDistributeSimdDirective
    //         DMP-NEXT:           OMPSharedClause {{.*}} <implicit>
    //         DMP-NEXT:             DeclRefExpr {{.*}} 'l' 'int'
    //              DMP:           ForStmt
    //
    //          PRT-AO-NEXT: // v----------ACC----------v
    //           PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(3)
    //       PRT-A-SRC-SAME: {{^ }}INDEPENDENT GANG
    //   PRT-A-AST-IND-SAME: {{^ }}independent
    //  PRT-A-AST-GANG-SAME: {{^ }}gang
    //           PRT-A-SAME: {{^ }}vector{{$}}
    //           PRT-A-NEXT: for (k ={{.*}}) {
    //           PRT-A-NEXT:   {{printf|TGT_PRINTF}}
    //           PRT-A-NEXT:   for (l ={{.*}})
    //           PRT-A-NEXT:     ;
    //           PRT-A-NEXT: }
    //          PRT-AO-NEXT: // ---------ACC->OMP--------
    //          PRT-AO-NEXT: // #pragma omp target teams num_teams(3) firstprivate(l){{$}}
    //          PRT-AO-NEXT: // {
    //          PRT-AO-NEXT: //   int k;
    //          PRT-AO-NEXT: //   #pragma omp distribute simd{{$}}
    //          PRT-AO-NEXT: //   for (k ={{.*}}) {
    //          PRT-AO-NEXT: //     {{printf|TGT_PRINTF}}
    //          PRT-AO-NEXT: //     for (l ={{.*}})
    //          PRT-AO-NEXT: //       ;
    //          PRT-AO-NEXT: //   }
    //          PRT-AO-NEXT: // }
    //          PRT-AO-NEXT: // ^----------OMP----------^
    //
    //          PRT-OA-NEXT: // v----------OMP----------v
    //           PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(3) firstprivate(l){{$}}
    //           PRT-O-NEXT: {
    //           PRT-O-NEXT:   int k;
    //           PRT-O-NEXT:   {{^ *}}#pragma omp distribute simd{{$}}
    //           PRT-O-NEXT:   for (k ={{.*}}) {
    //           PRT-O-NEXT:     {{printf|TGT_PRINTF}}
    //           PRT-O-NEXT:     for (l ={{.*}})
    //           PRT-O-NEXT:       ;
    //           PRT-O-NEXT:   }
    //          PRT-O-NEXT:  }
    //          PRT-OA-NEXT: // ---------OMP<-ACC--------
    //          PRT-OA-NEXT: // #pragma acc parallel loop num_gangs(3)
    //      PRT-OA-SRC-SAME: {{^ }}INDEPENDENT GANG
    //  PRT-OA-AST-IND-SAME: {{^ }}independent
    // PRT-OA-AST-GANG-SAME: {{^ }}gang
    //          PRT-OA-SAME: {{^ }}vector{{$}}
    //          PRT-OA-NEXT: // for (k ={{.*}}) {
    //          PRT-OA-NEXT: //   {{printf|TGT_PRINTF}}
    //          PRT-OA-NEXT: //   for (l ={{.*}})
    //          PRT-OA-NEXT: //     ;
    //          PRT-OA-NEXT: // }
    //          PRT-OA-NEXT: // ^----------ACC----------^
    //
    // PRT-NOACC-NEXT: for (k ={{.*}}) {
    // PRT-NOACC-NEXT:   {{printf|TGT_PRINTF}}
    // PRT-NOACC-NEXT:   for (l ={{.*}})
    // PRT-NOACC-NEXT:     ;
    // PRT-NOACC-NEXT: }
    //
    // Gang-partitioned mode executes each loop iteration once.
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    #pragma acc parallel loop num_gangs(3) INDEPENDENT GANG vector
    for (k = 1; k < 3; ++k) {
      TGT_PRINTF("loop iteration\n");
      for (l = 0; l < 2; ++l)
        ;
    }
  }

  //............................................................................
  // acc parallel loop INDEPENDENT GANG worker vector
  //............................................................................

  {
    int loopOnly;
    int loopOnlyArr[1];
    struct {
      bool loopOnlyFoundOldVal;
      bool loopOnlyFoundBadVal;

      bool loopOnlyArrFoundOldVal;
      bool loopOnlyArrFoundBadVal;
    } save;

    // DMP-LABEL: StringLiteral {{.*}} "declare loop var: acc parallel loop INDEPENDENT GANG worker vector\n"
    // PRT-LABEL: printf("declare loop var: acc parallel loop INDEPENDENT GANG worker vector\n");
    //   EXE-NOT: {{.}}
    // EXE-LABEL: declare loop var: acc parallel loop INDEPENDENT GANG worker vector{{$}}
    //   EXE-NOT: {{.}}
    memset(&save, 0, sizeof(save));
    loopOnly = 88;
    loopOnlyArr[0] = 88;
    printf("declare loop var: acc parallel loop INDEPENDENT GANG worker vector\n");

// FIXME: amdgcn misbehaves sometimes for worker loops.
// PRT-SRC-NEXT: #if !TGT_AMDGCN
#if !TGT_AMDGCN
    //              DMP: ACCParallelLoopDirective
    //         DMP-NEXT:   ACCNumGangsClause
    //         DMP-NEXT:     IntegerLiteral {{.*}} 'int' 3
    //     DMP-IND-NEXT:   ACCIndependentClause
    //      DMP-IND-NOT:     <implicit>
    //    DMP-GANG-NEXT:   ACCGangClause
    //     DMP-GANG-NOT:     <implicit>
    //         DMP-NEXT:   ACCWorkerClause
    //          DMP-NOT:     <implicit>
    //         DMP-NEXT:   ACCVectorClause
    //          DMP-NOT:     <implicit>
    //         DMP-NEXT:   effect: ACCParallelDirective
    //         DMP-NEXT:     ACCNumGangsClause
    //         DMP-NEXT:       IntegerLiteral {{.*}} 'int' 3
    //         DMP-NEXT:     ACCNomapClause {{.*}} <implicit>
    //         DMP-NEXT:       DeclRefExpr {{.*}} 'loopOnly' 'int'
    //         DMP-NEXT:     ACCCopyClause {{.*}} <implicit>
    //         DMP-NEXT:       DeclRefExpr {{.*}} 'save'
    //         DMP-NEXT:       DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    //         DMP-NEXT:     ACCSharedClause {{.*}} <implicit>
    //         DMP-NEXT:       DeclRefExpr {{.*}} 'save'
    //         DMP-NEXT:       DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    //         DMP-NEXT:     ACCFirstprivateClause {{.*}} <implicit>
    //         DMP-NEXT:       DeclRefExpr {{.*}} 'loopOnly' 'int'
    //         DMP-NEXT:     impl: OMPTargetTeamsDirective
    //         DMP-NEXT:       OMPNum_teamsClause
    //         DMP-NEXT:         IntegerLiteral {{.*}} 'int' 3
    //         DMP-NEXT:       OMPMapClause
    //          DMP-NOT:         <implicit>
    //         DMP-NEXT:         DeclRefExpr {{.*}} 'save'
    //         DMP-NEXT:         DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    //         DMP-NEXT:       OMPSharedClause
    //          DMP-NOT:         <implicit>
    //         DMP-NEXT:         DeclRefExpr {{.*}} 'save'
    //         DMP-NEXT:         DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    //         DMP-NEXT:       OMPFirstprivateClause
    //         DMP-NOT:          <implicit>
    //         DMP-NEXT:         DeclRefExpr {{.*}} 'loopOnly' 'int'
    //              DMP:     ACCLoopDirective
    //     DMP-IND-NEXT:       ACCIndependentClause
    //      DMP-IND-NOT:         <implicit>
    //    DMP-GANG-NEXT:       ACCGangClause
    //     DMP-GANG-NOT:         <implicit>
    //         DMP-NEXT:       ACCWorkerClause
    //          DMP-NOT:         <implicit>
    //         DMP-NEXT:       ACCVectorClause
    //          DMP-NOT:         <implicit>
    //  DMP-NO-IND-NEXT:       ACCIndependentClause {{.*}} <implicit>
    //         DMP-NEXT:       ACCSharedClause {{.*}} <implicit>
    //         DMP-NEXT:         DeclRefExpr {{.*}} 'save'
    //         DMP-NEXT:         DeclRefExpr {{.*}} 'loopOnly' 'int'
    //         DMP-NEXT:         DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    // DMP-NO-GANG-NEXT:       ACCGangClause {{.*}} <implicit>
    //         DMP-NEXT:       impl: OMPDistributeParallelForSimdDirective
    //         DMP-NEXT:         OMPSharedClause
    //          DMP-NOT:           <implicit>
    //         DMP-NEXT:           DeclRefExpr {{.*}} 'save'
    //         DMP-NEXT:           DeclRefExpr {{.*}} 'loopOnly' 'int'
    //         DMP-NEXT:           DeclRefExpr {{.*}} 'loopOnlyArr' 'int[1]'
    //              DMP:         ForStmt
    //
    //           PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(3)
    //       PRT-A-SRC-SAME: {{^ }}INDEPENDENT GANG
    //   PRT-A-AST-IND-SAME: {{^ }}independent
    //  PRT-A-AST-GANG-SAME: {{^ }}gang
    //           PRT-A-SAME: {{^ }}worker vector{{$}}
    //          PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(3) map(ompx_hold,tofrom: save,loopOnlyArr) shared(save,loopOnlyArr) firstprivate(loopOnly){{$}}
    //          PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for simd shared(save,loopOnly,loopOnlyArr){{$}}
    //           PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(3) map(ompx_hold,tofrom: save,loopOnlyArr) shared(save,loopOnlyArr) firstprivate(loopOnly){{$}}
    //           PRT-O-NEXT: {{^ *}}#pragma omp distribute parallel for simd shared(save,loopOnly,loopOnlyArr){{$}}
    //          PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(3)
    //      PRT-OA-SRC-SAME: {{^ }}INDEPENDENT GANG
    //  PRT-OA-AST-IND-SAME: {{^ }}independent
    // PRT-OA-AST-GANG-SAME: {{^ }}gang
    //          PRT-OA-SAME: {{^ }}worker vector{{$}}
    //             PRT-NEXT: for ({{.*}}) {
    //                  PRT: }
    //
    // Gang-partitioned mode executes each loop iteration once.
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    #pragma acc parallel loop num_gangs(3) INDEPENDENT GANG worker vector
    for (int i = 0; i < 2; ++i) {
      TGT_PRINTF("loop iteration\n");
      // We omit checking of an inner j loop this time.  j would be shared
      // among workers and vector lanes and thus would have an unpredictable
      // value.
      checkLoopPartShared(save, i, loopOnly, loopOnly, 88, 77);
      loopOnly = 77;
      checkLoopPartShared(save, i, loopOnlyArr, loopOnlyArr[0], 88, 77);
      loopOnlyArr[0] = 77;
    }
#endif

    // EXE-NEXT: save.loopOnlyFoundOldVal=1
    // EXE-NEXT: save.loopOnlyFoundBadVal=0
    // EXE-NEXT: save.loopOnlyArrFoundOldVal=1
    // EXE-NEXT: save.loopOnlyArrFoundBadVal=0
    // EXE-NEXT: loopOnly=88
    // EXE-NEXT: loopOnlyArr[0]=77
// FIXME: amdgcn misbehaves sometimes for worker loops.
#if !TGT_AMDGCN
    printf("save.loopOnlyFoundOldVal=%d\n", save.loopOnlyFoundOldVal);
    printf("save.loopOnlyFoundBadVal=%d\n", save.loopOnlyFoundBadVal);
    printf("save.loopOnlyArrFoundOldVal=%d\n", save.loopOnlyArrFoundOldVal);
    printf("save.loopOnlyArrFoundBadVal=%d\n", save.loopOnlyArrFoundBadVal);
    printf("loopOnly=%d\n", loopOnly);
    printf("loopOnlyArr[0]=%d\n", loopOnlyArr[0]);
#else
    printf("save.loopOnlyFoundOldVal=%d\n", 1);
    printf("save.loopOnlyFoundBadVal=%d\n", 0);
    printf("save.loopOnlyArrFoundOldVal=%d\n", 1);
    printf("save.loopOnlyArrFoundBadVal=%d\n", 0);
    printf("loopOnly=%d\n", 88);
    printf("loopOnlyArr[0]=%d\n", 77);
#endif
  }

  {
    // DMP-LABEL: StringLiteral {{.*}} "assigned loop var: acc parallel loop INDEPENDENT GANG worker vector\n"
    // PRT-LABEL: printf("assigned loop var: acc parallel loop INDEPENDENT GANG worker vector\n");
    //   EXE-NOT: {{.}}
    // EXE-LABEL: assigned loop var: acc parallel loop INDEPENDENT GANG worker vector{{$}}
    //   EXE-NOT: {{.}}
    int k, l;
    printf("assigned loop var: acc parallel loop INDEPENDENT GANG worker vector\n");

// FIXME: amdgcn misbehaves sometimes for worker loops.
// PRT-SRC-NEXT: #if !TGT_AMDGCN
#if !TGT_AMDGCN
    //              DMP: ACCParallelLoopDirective
    //         DMP-NEXT:   ACCNumGangsClause
    //         DMP-NEXT:     IntegerLiteral {{.*}} 'int' 3
    //     DMP-IND-NEXT:   ACCIndependentClause
    //      DMP-IND-NOT:     <implicit>
    //    DMP-GANG-NEXT:   ACCGangClause
    //     DMP-GANG-NOT:     <implicit>
    //         DMP-NEXT:   ACCWorkerClause
    //          DMP-NOT:     <implicit>
    //         DMP-NEXT:   ACCVectorClause
    //          DMP-NOT:     <implicit>
    //         DMP-NEXT:   effect: ACCParallelDirective
    //         DMP-NEXT:     ACCNumGangsClause
    //         DMP-NEXT:       IntegerLiteral {{.*}} 'int' 3
    //         DMP-NEXT:     ACCNomapClause {{.*}} <implicit>
    //         DMP-NEXT:       DeclRefExpr {{.*}} 'l' 'int'
    //         DMP-NEXT:     ACCFirstprivateClause {{.*}} <implicit>
    //         DMP-NEXT:       DeclRefExpr {{.*}} 'l' 'int'
    //         DMP-NEXT:     impl: OMPTargetTeamsDirective
    //         DMP-NEXT:       OMPNum_teamsClause
    //         DMP-NEXT:         IntegerLiteral {{.*}} 'int' 3
    //         DMP-NEXT:       OMPFirstprivateClause
    //          DMP-NOT:         <implicit>
    //         DMP-NEXT:         DeclRefExpr {{.*}} 'l' 'int'
    //              DMP:     ACCLoopDirective
    //     DMP-IND-NEXT:       ACCIndependentClause
    //      DMP-IND-NOT:         <implicit>
    //    DMP-GANG-NEXT:       ACCGangClause
    //     DMP-GANG-NOT:         <implicit>
    //         DMP-NEXT:       ACCWorkerClause
    //          DMP-NOT:         <implicit>
    //         DMP-NEXT:       ACCVectorClause
    //          DMP-NOT:         <implicit>
    //  DMP-NO-IND-NEXT:       ACCIndependentClause {{.*}} <implicit>
    //         DMP-NEXT:       ACCPrivateClause {{.*}} <predetermined>
    //         DMP-NEXT:         DeclRefExpr {{.*}} 'k' 'int'
    //         DMP-NEXT:       ACCSharedClause {{.*}} <implicit>
    //         DMP-NEXT:         DeclRefExpr {{.*}} 'l' 'int'
    // DMP-NO-GANG-NEXT:       ACCGangClause {{.*}} <implicit>
    //         DMP-NEXT:       impl: CompoundStmt
    //         DMP-NEXT:         DeclStmt
    //         DMP-NEXT:           VarDecl {{.*}} k 'int'
    //         DMP-NEXT:         OMPDistributeParallelForSimdDirective
    //         DMP-NEXT:           OMPSharedClause
    //          DMP-NOT:             <implicit>
    //         DMP-NEXT:             DeclRefExpr {{.*}} 'l' 'int'
    //              DMP:           ForStmt
    //
    //          PRT-AO-NEXT: // v----------ACC----------v
    //           PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(3)
    //       PRT-A-SRC-SAME: {{^ }}INDEPENDENT GANG
    //   PRT-A-AST-IND-SAME: {{^ }}independent
    //  PRT-A-AST-GANG-SAME: {{^ }}gang
    //           PRT-A-SAME: {{^ }}worker vector{{$}}
    //           PRT-A-NEXT: for (k ={{.*}}) {
    //           PRT-A-NEXT:   {{printf|TGT_PRINTF}}
    //           PRT-A-NEXT:   for (l ={{.*}})
    //           PRT-A-NEXT:     ;
    //           PRT-A-NEXT: }
    //          PRT-AO-NEXT: // ---------ACC->OMP--------
    //          PRT-AO-NEXT: // #pragma omp target teams num_teams(3) firstprivate(l){{$}}
    //          PRT-AO-NEXT: // {
    //          PRT-AO-NEXT: //   int k;
    //          PRT-AO-NEXT: //   #pragma omp distribute parallel for simd shared(l){{$}}
    //          PRT-AO-NEXT: //   for (k ={{.*}}) {
    //          PRT-AO-NEXT: //     {{printf|TGT_PRINTF}}
    //          PRT-AO-NEXT: //     for (l ={{.*}})
    //          PRT-AO-NEXT: //       ;
    //          PRT-AO-NEXT: //   }
    //          PRT-AO-NEXT: // }
    //          PRT-AO-NEXT: // ^----------OMP----------^
    //
    //          PRT-OA-NEXT: // v----------OMP----------v
    //           PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(3) firstprivate(l){{$}}
    //           PRT-O-NEXT: {
    //           PRT-O-NEXT:   int k;
    //           PRT-O-NEXT:   {{^ *}}#pragma omp distribute parallel for simd shared(l){{$}}
    //           PRT-O-NEXT:   for (k ={{.*}}) {
    //           PRT-O-NEXT:     {{printf|TGT_PRINTF}}
    //           PRT-O-NEXT:     for (l ={{.*}})
    //           PRT-O-NEXT:       ;
    //           PRT-O-NEXT:   }
    //          PRT-O-NEXT:  }
    //          PRT-OA-NEXT: // ---------OMP<-ACC--------
    //          PRT-OA-NEXT: // #pragma acc parallel loop num_gangs(3)
    //      PRT-OA-SRC-SAME: {{^ }}INDEPENDENT GANG
    //  PRT-OA-AST-IND-SAME: {{^ }}independent
    // PRT-OA-AST-GANG-SAME: {{^ }}gang
    //          PRT-OA-SAME: {{^ }}worker vector{{$}}
    //          PRT-OA-NEXT: // for (k ={{.*}}) {
    //          PRT-OA-NEXT: //   {{printf|TGT_PRINTF}}
    //          PRT-OA-NEXT: //   for (l ={{.*}})
    //          PRT-OA-NEXT: //     ;
    //          PRT-OA-NEXT: // }
    //          PRT-OA-NEXT: // ^----------ACC----------^
    //
    // PRT-NOACC-NEXT: for (k ={{.*}}) {
    // PRT-NOACC-NEXT:   {{printf|TGT_PRINTF}}
    // PRT-NOACC-NEXT:   for (l ={{.*}})
    // PRT-NOACC-NEXT:     ;
    // PRT-NOACC-NEXT: }
    //
    // Gang-partitioned mode executes each loop iteration once.
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    #pragma acc parallel loop num_gangs(3) INDEPENDENT GANG worker vector
    for (k = 1; k < 3; ++k) {
      TGT_PRINTF("loop iteration\n");
      for (l = 0; l < 2; ++l)
        ;
    }
#endif
  }

  return 0;
}

// EXE-NOT: {{.}}