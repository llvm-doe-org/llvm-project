// Check correct loop partitioning and implicit data attributes on
// "acc loop" within "acc parallel" or on effective "acc loop" from
// "acc parallel loop".
//
// Implicit data attributes for member expressions are checked in
// loop-da-member-expr.c and loop-da-member-expr.cpp.
//
// When independent and gang are omitted, they're usually implicit.  Thus, test
// code is almost identical across these cases, so it's easier to parameterize
// the test code instead of repeat it entirely with minor differences.

// REDEFINE: %{all:clang:args} = -DINDEPENDENT=independent -DGANG=gang
// REDEFINE: %{all:fc:pres} = IND,GANG
// RUN: %{acc-check-dmp}
// RUN: %{acc-check-prt}
// RUN: %{acc-check-exe}

// REDEFINE: %{all:clang:args} = -DINDEPENDENT= -DGANG=gang
// REDEFINE: %{all:fc:pres} = NO-IND,GANG
// RUN: %{acc-check-dmp}
// RUN: %{acc-check-prt}
// RUN: %{acc-check-exe}

// REDEFINE: %{all:clang:args} = -DINDEPENDENT=independent -DGANG=
// REDEFINE: %{all:fc:pres} = IND,NO-GANG
// RUN: %{acc-check-dmp}
// RUN: %{acc-check-prt}
// RUN: %{acc-check-exe}

// REDEFINE: %{all:clang:args} = -DINDEPENDENT= -DGANG=
// REDEFINE: %{all:fc:pres} = NO-IND,NO-GANG
// RUN: %{acc-check-dmp}
// RUN: %{acc-check-prt}
// RUN: %{acc-check-exe}

// END.

/* expected-no-diagnostics */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if TGT_USE_STDIO
# define TGT_PRINTF(...) printf(__VA_ARGS__)
#else
# define TGT_PRINTF(...)
#endif

struct Save {
  bool loopOnlyAtI0FoundBadVal;
  bool loopOnlyAtI1FoundBadVal;
  bool loopOnlyFoundOldVal;
  bool loopOnlyFoundBadVal;

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
};

static void withinGangFnAccLoopSeqRun();
#pragma acc routine gang
static void withinGangFnAccLoopSeq(struct Save *save, int *loopOnly,
                                   int *loopOnlyArr);

static void withinGangFnAccLoopAutoGangRun();
#pragma acc routine gang
static void withinGangFnAccLoopAutoGang(struct Save *save, int *loopOnly,
                                        int *loopOnlyArr);
static void withinGangFnAccLoopAutoGangWorkerRun();
#pragma acc routine gang
static void withinGangFnAccLoopAutoGangWorker(struct Save *save, int *loopOnly,
                                              int *loopOnlyArr);
static void withinGangFnAccLoopAutoGangVectorRun();
#pragma acc routine gang
static void withinGangFnAccLoopAutoGangVector(struct Save *save, int *loopOnly,
                                              int *loopOnlyArr);
static void withinGangFnAccLoopAutoGangWorkerVectorRun();
#pragma acc routine gang
static void withinGangFnAccLoopAutoGangWorkerVector(struct Save *save,
                                                    int *loopOnly,
                                                    int *loopOnlyArr);
static void withinGangFnAccLoopIndependentGangRun();
#pragma acc routine gang
static void withinGangFnAccLoopIndependentGang(struct Save *save, int *loopOnly,
                                               int *loopOnlyArr);
static void withinGangFnAccLoopIndependentGangWorkerRun();
#pragma acc routine gang
static void withinGangFnAccLoopIndependentGangWorker(struct Save *save,
                                                     int *loopOnly,
                                                     int *loopOnlyArr);
static void withinGangFnAccLoopIndependentGangVectorRun();
#pragma acc routine gang
static void withinGangFnAccLoopIndependentGangVector(struct Save *save,
                                                     int *loopOnly,
                                                     int *loopOnlyArr);
static void withinGangFnAccLoopIndependentGangWorkerVectorRun();
#pragma acc routine gang
static void withinGangFnAccLoopIndependentGangWorkerVector(struct Save *save,
                                                           int *loopOnly,
                                                           int *loopOnlyArr);

static void withinWorkerFnAccLoopSeqRun();
#pragma acc routine worker
static void withinWorkerFnAccLoopSeq(struct Save *save, int *loopOnly,
                                     int *loopOnlyArr);
static void withinWorkerFnAccLoopAutoRun();
#pragma acc routine worker
static void withinWorkerFnAccLoopAuto(struct Save *save, int *loopOnly,
                                      int *loopOnlyArr);
static void withinWorkerFnAccLoopAutoWorkerRun();
#pragma acc routine worker
static void withinWorkerFnAccLoopAutoWorker(struct Save *save, int *loopOnly,
                                            int *loopOnlyArr);
static void withinWorkerFnAccLoopAutoVectorRun();
#pragma acc routine worker
static void withinWorkerFnAccLoopAutoVector(struct Save *save, int *loopOnly,
                                            int *loopOnlyArr);
static void withinWorkerFnAccLoopAutoWorkerVectorRun();
#pragma acc routine worker
static void withinWorkerFnAccLoopAutoWorkerVector(struct Save *save,
                                                  int *loopOnly,
                                                  int *loopOnlyArr);
static void withinWorkerFnAccLoopIndependentRun();
#pragma acc routine worker
static void withinWorkerFnAccLoopIndependent(struct Save *save, int *loopOnly,
                                             int *loopOnlyArr);
static void withinWorkerFnAccLoopIndependentWorkerRun();
#pragma acc routine worker
static void withinWorkerFnAccLoopIndependentWorker(struct Save *save,
                                                   int *loopOnly,
                                                   int *loopOnlyArr);
static void withinWorkerFnAccLoopIndependentVectorRun();
#pragma acc routine worker
static void withinWorkerFnAccLoopIndependentVector(struct Save *save,
                                                   int *loopOnly,
                                                   int *loopOnlyArr);
static void withinWorkerFnAccLoopIndependentWorkerVectorRun();
#pragma acc routine worker
static void withinWorkerFnAccLoopIndependentWorkerVector(struct Save *save,
                                                         int *loopOnly,
                                                         int *loopOnlyArr);

static void withinVectorFnAccLoopSeqRun();
#pragma acc routine vector
static void withinVectorFnAccLoopSeq(struct Save *save, int *loopOnly,
                                     int *loopOnlyArr);
static void withinVectorFnAccLoopAutoRun();
#pragma acc routine vector
static void withinVectorFnAccLoopAuto(struct Save *save, int *loopOnly,
                                      int *loopOnlyArr);
static void withinVectorFnAccLoopAutoVectorRun();
#pragma acc routine vector
static void withinVectorFnAccLoopAutoVector(struct Save *save, int *loopOnly,
                                            int *loopOnlyArr);
static void withinVectorFnAccLoopIndependentRun();
#pragma acc routine vector
static void withinVectorFnAccLoopIndependent(struct Save *save, int *loopOnly,
                                             int *loopOnlyArr);
static void withinVectorFnAccLoopIndependentVectorRun();
#pragma acc routine vector
static void withinVectorFnAccLoopIndependentVector(struct Save *save,
                                                   int *loopOnly,
                                                   int *loopOnlyArr);

static void withinSeqFnAccLoopSeqRun();
#pragma acc routine seq
static void withinSeqFnAccLoopSeq(struct Save *save, int *loopOnly,
                                  int *loopOnlyArr);
static void withinSeqFnAccLoopAutoRun();
#pragma acc routine seq
static void withinSeqFnAccLoopAuto(struct Save *save, int *loopOnly,
                                   int *loopOnlyArr);
static void withinSeqFnAccLoopIndependentRun();
#pragma acc routine seq
static void withinSeqFnAccLoopIndependent(struct Save *save, int *loopOnly,
                                          int *loopOnlyArr);

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
#define checkLoopSeqPrivate(Result, LoopIdx, VarName, VarVal, OldVal, NewVal)  \
  checkLoopSeqPrivate_(&((Result).VarName ## AtI0FoundBadVal),                 \
                       &((Result).VarName ## AtI1FoundBadVal),                 \
                       (LoopIdx), VarVal, (OldVal), (NewVal))
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
#define checkLoopPartPrivate(Result, LoopIdx, VarName, VarVal, OldVal, NewVal) \
  checkLoopPartPrivate_(&((Result).VarName ## AtI0FoundBadVal),                \
                        &((Result).VarName ## AtI1FoundBadVal),                \
                        (LoopIdx), VarVal, (OldVal), (NewVal))
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
    struct Save save;

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
    // DMP-NEXT:     OMPFirstprivateClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'loopOnly' 'int'
    //      DMP:     CompoundStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc parallel num_gangs(3){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(3) map(ompx_hold,tofrom: save,loopOnlyArr) firstprivate(loopOnly){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(3) map(ompx_hold,tofrom: save,loopOnlyArr) firstprivate(loopOnly){{$}}
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
        checkLoopSeqPrivate(save, i, loopOnly, loopOnly, 88, 77);
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
    //      DMP-NEXT:     OMPFirstprivateClause
    //       DMP-NOT:       <implicit>
    //      DMP-NEXT:       DeclRefExpr {{.*}} 'loopOnly' 'int'
    //           DMP:     CompoundStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc parallel num_gangs(3){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(3) map(ompx_hold,tofrom: save,loopOnlyArr) firstprivate(loopOnly){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(3) map(ompx_hold,tofrom: save,loopOnlyArr) firstprivate(loopOnly){{$}}
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
        checkLoopSeqPrivate(save, i, loopOnly, loopOnly, 88, 77);
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
    // DMP-NEXT:     OMPFirstprivateClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'loopOnly' 'int'
    //      DMP:     CompoundStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc parallel num_gangs(3){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(3) map(ompx_hold,tofrom: save,loopOnlyArr) firstprivate(loopOnly){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(3) map(ompx_hold,tofrom: save,loopOnlyArr) firstprivate(loopOnly){{$}}
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
        checkLoopSeqPrivate(save, i, loopOnly, loopOnly, 88, 77);
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
    // DMP-NEXT:     OMPFirstprivateClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'loopOnly' 'int'
    //      DMP:     CompoundStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc parallel num_gangs(3){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(3) map(ompx_hold,tofrom: save,loopOnlyArr) firstprivate(loopOnly){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(3) map(ompx_hold,tofrom: save,loopOnlyArr) firstprivate(loopOnly){{$}}
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
        checkLoopSeqPrivate(save, i, loopOnly, loopOnly, 88, 77);
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
    // DMP-NEXT:     OMPFirstprivateClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'loopOnly' 'int'
    //      DMP:     CompoundStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc parallel num_gangs(3){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(3) map(ompx_hold,tofrom: save,loopOnlyArr) firstprivate(loopOnly){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(3) map(ompx_hold,tofrom: save,loopOnlyArr) firstprivate(loopOnly){{$}}
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
        checkLoopSeqPrivate(save, i, loopOnly, loopOnly, 88, 77);
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
    struct Save save;

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
    // DMP-NEXT:     OMPFirstprivateClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'loopOnly' 'int'
    //      DMP:     CompoundStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc parallel num_gangs(3){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(3) map(ompx_hold,tofrom: save,loopOnlyArr) firstprivate(loopOnly){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(3) map(ompx_hold,tofrom: save,loopOnlyArr) firstprivate(loopOnly){{$}}
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
      //          DMP-NOT:     OMP
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
        checkLoopPartPrivate(save, i, loopOnly, loopOnly, 88, 77);
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
      //          DMP-NOT:     OMP
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
      //          DMP-NOT:     OMP
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
    struct Save save;

    // DMP-LABEL: StringLiteral {{.*}} "acc loop INDEPENDENT GANG worker\n"
    // PRT-LABEL: printf("acc loop INDEPENDENT GANG worker\n");
    //   EXE-NOT: {{.}}
    // EXE-LABEL: acc loop INDEPENDENT GANG worker{{$}}
    //   EXE-NOT: {{.}}
    memset(&save, 0, sizeof(save));
    loopOnly = 88;
    loopOnlyArr[0] = 88;
    printf("acc loop INDEPENDENT GANG worker\n");

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
    // DMP-NEXT:     OMPFirstprivateClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'loopOnly' 'int'
    //      DMP:     CompoundStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc parallel num_gangs(3){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(3) map(ompx_hold,tofrom: save,loopOnlyArr) firstprivate(loopOnly){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(3) map(ompx_hold,tofrom: save,loopOnlyArr) firstprivate(loopOnly){{$}}
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
      //          DMP-NOT:     OMP
      //              DMP:     ForStmt
      //
      //           PRT-A-NEXT: {{^ *}}#pragma acc loop
      //       PRT-A-SRC-SAME: {{^ }}INDEPENDENT GANG
      //   PRT-A-AST-IND-SAME: {{^ }}independent
      //  PRT-A-AST-GANG-SAME: {{^ }}gang
      //           PRT-A-SAME: {{^ }}worker{{$}}
      //          PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for{{$}}
      //           PRT-O-NEXT: {{^ *}}#pragma omp distribute parallel for{{$}}
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
      //          DMP-NOT:     OMP
      //              DMP:     ForStmt
      //
      //           PRT-A-NEXT: {{^ *}}#pragma acc loop
      //       PRT-A-SRC-SAME: {{^ }}INDEPENDENT GANG
      //   PRT-A-AST-IND-SAME: {{^ }}independent
      //  PRT-A-AST-GANG-SAME: {{^ }}gang
      //           PRT-A-SAME: {{^ }}worker{{$}}
      //          PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for{{$}}
      //           PRT-O-NEXT: {{^ *}}#pragma omp distribute parallel for{{$}}
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
  // acc loop INDEPENDENT GANG vector
  //............................................................................

  {
    int loopOnly;
    int loopOnlyArr[1];
    struct Save save;

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
    // DMP-NEXT:     OMPFirstprivateClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'loopOnly' 'int'
    //      DMP:     CompoundStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc parallel num_gangs(3){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(3) map(ompx_hold,tofrom: save,loopOnlyArr) firstprivate(loopOnly){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(3) map(ompx_hold,tofrom: save,loopOnlyArr) firstprivate(loopOnly){{$}}
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
      //          DMP-NOT:     OMP
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
      //          DMP-NOT:     OMP
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
    struct Save save;

    // DMP-LABEL: StringLiteral {{.*}} "acc loop INDEPENDENT GANG worker vector\n"
    // PRT-LABEL: printf("acc loop INDEPENDENT GANG worker vector\n");
    //   EXE-NOT: {{.}}
    // EXE-LABEL: acc loop INDEPENDENT GANG worker vector{{$}}
    //   EXE-NOT: {{.}}
    memset(&save, 0, sizeof(save));
    loopOnly = 88;
    loopOnlyArr[0] = 88;
    printf("acc loop INDEPENDENT GANG worker vector\n");

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
    // DMP-NEXT:     OMPFirstprivateClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'loopOnly' 'int'
    //      DMP:     CompoundStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc parallel num_gangs(3){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(3) map(ompx_hold,tofrom: save,loopOnlyArr) firstprivate(loopOnly){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(3) map(ompx_hold,tofrom: save,loopOnlyArr) firstprivate(loopOnly){{$}}
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
      //          DMP-NOT:     OMP
      //              DMP:     ForStmt
      //
      //           PRT-A-NEXT: {{^ *}}#pragma acc loop
      //       PRT-A-SRC-SAME: {{^ }}INDEPENDENT GANG
      //   PRT-A-AST-IND-SAME: {{^ }}independent
      //  PRT-A-AST-GANG-SAME: {{^ }}gang
      //           PRT-A-SAME: {{^ }}worker vector{{$}}
      //          PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for simd{{$}}
      //           PRT-O-NEXT: {{^ *}}#pragma omp distribute parallel for simd{{$}}
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
      //          DMP-NOT:     OMP
      //              DMP:     ForStmt
      //
      //           PRT-A-NEXT: {{^ *}}#pragma acc loop
      //       PRT-A-SRC-SAME: {{^ }}INDEPENDENT GANG
      //   PRT-A-AST-IND-SAME: {{^ }}independent
      //  PRT-A-AST-GANG-SAME: {{^ }}gang
      //           PRT-A-SAME: {{^ }}worker vector{{$}}
      //          PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for simd{{$}}
      //           PRT-O-NEXT: {{^ *}}#pragma omp distribute parallel for simd{{$}}
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
    struct Save save;

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
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(3) map(ompx_hold,tofrom: save,loopOnlyArr) firstprivate(j,loopOnly){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(3) map(ompx_hold,tofrom: save,loopOnlyArr) firstprivate(j,loopOnly){{$}}
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
      checkLoopSeqPrivate(save, i, loopOnly, loopOnly, 88, 77);
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
    //          PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(3) map(ompx_hold,tofrom: save,loopOnlyArr) firstprivate(j,loopOnly){{$}}
    //           PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(3) map(ompx_hold,tofrom: save,loopOnlyArr) firstprivate(j,loopOnly){{$}}
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
      checkLoopSeqPrivate(save, i, loopOnly, loopOnly, 88, 77);
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
    //          PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(3) map(ompx_hold,tofrom: save,loopOnlyArr) firstprivate(j,loopOnly){{$}}
    //           PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(3) map(ompx_hold,tofrom: save,loopOnlyArr) firstprivate(j,loopOnly){{$}}
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
      checkLoopSeqPrivate(save, i, loopOnly, loopOnly, 88, 77);
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
    //          PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(3) map(ompx_hold,tofrom: save,loopOnlyArr) firstprivate(j,loopOnly){{$}}
    //           PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(3) map(ompx_hold,tofrom: save,loopOnlyArr) firstprivate(j,loopOnly){{$}}
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
      checkLoopSeqPrivate(save, i, loopOnly, loopOnly, 88, 77);
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
    //          PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(3) map(ompx_hold,tofrom: save,loopOnlyArr) firstprivate(j,loopOnly){{$}}
    //           PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(3) map(ompx_hold,tofrom: save,loopOnlyArr) firstprivate(j,loopOnly){{$}}
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
      checkLoopSeqPrivate(save, i, loopOnly, loopOnly, 88, 77);
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
    struct Save save;

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
    //          DMP-NOT:         OMP
    //              DMP:         ForStmt
    //
    //           PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(3)
    //       PRT-A-SRC-SAME: {{^ }}INDEPENDENT GANG
    //   PRT-A-AST-IND-SAME: {{^ }}independent
    //  PRT-A-AST-GANG-SAME: {{^ }}gang
    //           PRT-A-SAME: {{^$}}
    //          PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(3) map(ompx_hold,tofrom: save,loopOnlyArr) firstprivate(j,loopOnly){{$}}
    //          PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
    //           PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(3) map(ompx_hold,tofrom: save,loopOnlyArr) firstprivate(j,loopOnly){{$}}
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
      checkLoopPartPrivate(save, i, loopOnly, loopOnly, 88, 77);
      loopOnly = 77;
      checkLoopPartShared(save, i, loopOnlyArr, loopOnlyArr[0], 88, 77);
      loopOnlyArr[0] = 77;
      // Each gang that is assigned one or more i iterations will see the final
      // j from the j loop above, and the remaining gangs will not.
      if (j == 0)
        save.declNestedLoopFoundOldVal = true;
      else if (j == 2)
        save.declNestedLoopFoundNewVal = true;
      else
        save.declNestedLoopFoundBadVal = true;
    }

    // EXE-NEXT: save.loopOnlyAtI0FoundBadVal=0
    // EXE-NEXT: save.loopOnlyAtI1FoundBadVal=0
    // EXE-NEXT: save.loopOnlyArrFoundOldVal=1
    // EXE-NEXT: save.loopOnlyArrFoundBadVal=0
    // EXE-NEXT: save.declNestedLoopFoundOldVal=0
    // EXE-NEXT: save.declNestedLoopFoundNewVal=1
    // EXE-NEXT: save.declNestedLoopFoundBadVal=0
    // EXE-NEXT: loopOnly=88
    // EXE-NEXT: loopOnlyArr[0]=77
    printf("save.loopOnlyAtI0FoundBadVal=%d\n", save.loopOnlyAtI0FoundBadVal);
    printf("save.loopOnlyAtI1FoundBadVal=%d\n", save.loopOnlyAtI1FoundBadVal);
    printf("save.loopOnlyArrFoundOldVal=%d\n", save.loopOnlyArrFoundOldVal);
    printf("save.loopOnlyArrFoundBadVal=%d\n", save.loopOnlyArrFoundBadVal);
    printf("save.declNestedLoopFoundOldVal=%d\n", save.declNestedLoopFoundOldVal);
    printf("save.declNestedLoopFoundNewVal=%d\n", save.declNestedLoopFoundNewVal);
    printf("save.declNestedLoopFoundBadVal=%d\n", save.declNestedLoopFoundBadVal);
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
    //          DMP-NOT:         OMP
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
    struct Save save;

    // DMP-LABEL: StringLiteral {{.*}} "declared loop var: acc parallel loop INDEPENDENT GANG worker\n"
    // PRT-LABEL: printf("declared loop var: acc parallel loop INDEPENDENT GANG worker\n");
    //   EXE-NOT: {{.}}
    // EXE-LABEL: declared loop var: acc parallel loop INDEPENDENT GANG worker{{$}}
    //   EXE-NOT: {{.}}
    memset(&save, 0, sizeof(save));
    loopOnly = 88;
    loopOnlyArr[0] = 88;
    printf("declared loop var: acc parallel loop INDEPENDENT GANG worker\n");

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
    //          DMP-NOT:         OMP
    //              DMP:         ForStmt
    //
    //           PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(3)
    //       PRT-A-SRC-SAME: {{^ }}INDEPENDENT GANG
    //   PRT-A-AST-IND-SAME: {{^ }}independent
    //  PRT-A-AST-GANG-SAME: {{^ }}gang
    //           PRT-A-SAME: {{^ }}worker{{$}}
    //          PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(3) map(ompx_hold,tofrom: save,loopOnlyArr) firstprivate(loopOnly){{$}}
    //          PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for{{$}}
    //           PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(3) map(ompx_hold,tofrom: save,loopOnlyArr) firstprivate(loopOnly){{$}}
    //           PRT-O-NEXT: {{^ *}}#pragma omp distribute parallel for{{$}}
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
    // DMP-LABEL: StringLiteral {{.*}} "assigned loop var: acc parallel loop INDEPENDENT GANG worker\n"
    // PRT-LABEL: printf("assigned loop var: acc parallel loop INDEPENDENT GANG worker\n");
    //   EXE-NOT: {{.}}
    // EXE-LABEL: assigned loop var: acc parallel loop INDEPENDENT GANG worker{{$}}
    //   EXE-NOT: {{.}}
    int k, l;
    printf("assigned loop var: acc parallel loop INDEPENDENT GANG worker\n");

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
    //          DMP-NOT:         OMP
    //              DMP:         ForStmt
    //
    //           PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(3)
    //       PRT-A-SRC-SAME: {{^ }}INDEPENDENT GANG
    //   PRT-A-AST-IND-SAME: {{^ }}independent
    //  PRT-A-AST-GANG-SAME: {{^ }}gang
    //           PRT-A-SAME: {{^ }}worker{{$}}
    //          PRT-AO-NEXT: // #pragma omp target teams num_teams(3) firstprivate(l){{$}}
    //          PRT-AO-NEXT: // #pragma omp distribute parallel for private(k){{$}}
    //           PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(3) firstprivate(l){{$}}
    //           PRT-O-NEXT: {{^ *}}#pragma omp distribute parallel for private(k){{$}}
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
  }

  //............................................................................
  // acc parallel loop INDEPENDENT GANG vector
  //............................................................................

  {
    int loopOnly;
    int loopOnlyArr[1];
    struct Save save;

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
    //          DMP-NOT:         OMP
    //              DMP:         ForStmt
    //
    //           PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(3)
    //       PRT-A-SRC-SAME: {{^ }}INDEPENDENT GANG
    //   PRT-A-AST-IND-SAME: {{^ }}independent
    //  PRT-A-AST-GANG-SAME: {{^ }}gang
    //           PRT-A-SAME: {{^ }}vector{{$}}
    //          PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(3) map(ompx_hold,tofrom: save,loopOnlyArr) firstprivate(loopOnly){{$}}
    //          PRT-AO-NEXT: {{^ *}}// #pragma omp distribute simd{{$}}
    //           PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(3) map(ompx_hold,tofrom: save,loopOnlyArr) firstprivate(loopOnly){{$}}
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
    //          DMP-NOT:           OMP
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
    struct Save save;

    // DMP-LABEL: StringLiteral {{.*}} "declare loop var: acc parallel loop INDEPENDENT GANG worker vector\n"
    // PRT-LABEL: printf("declare loop var: acc parallel loop INDEPENDENT GANG worker vector\n");
    //   EXE-NOT: {{.}}
    // EXE-LABEL: declare loop var: acc parallel loop INDEPENDENT GANG worker vector{{$}}
    //   EXE-NOT: {{.}}
    memset(&save, 0, sizeof(save));
    loopOnly = 88;
    loopOnlyArr[0] = 88;
    printf("declare loop var: acc parallel loop INDEPENDENT GANG worker vector\n");

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
    //          DMP-NOT:         OMP
    //              DMP:         ForStmt
    //
    //           PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(3)
    //       PRT-A-SRC-SAME: {{^ }}INDEPENDENT GANG
    //   PRT-A-AST-IND-SAME: {{^ }}independent
    //  PRT-A-AST-GANG-SAME: {{^ }}gang
    //           PRT-A-SAME: {{^ }}worker vector{{$}}
    //          PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(3) map(ompx_hold,tofrom: save,loopOnlyArr) firstprivate(loopOnly){{$}}
    //          PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for simd{{$}}
    //           PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(3) map(ompx_hold,tofrom: save,loopOnlyArr) firstprivate(loopOnly){{$}}
    //           PRT-O-NEXT: {{^ *}}#pragma omp distribute parallel for simd{{$}}
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
    // DMP-LABEL: StringLiteral {{.*}} "assigned loop var: acc parallel loop INDEPENDENT GANG worker vector\n"
    // PRT-LABEL: printf("assigned loop var: acc parallel loop INDEPENDENT GANG worker vector\n");
    //   EXE-NOT: {{.}}
    // EXE-LABEL: assigned loop var: acc parallel loop INDEPENDENT GANG worker vector{{$}}
    //   EXE-NOT: {{.}}
    int k, l;
    printf("assigned loop var: acc parallel loop INDEPENDENT GANG worker vector\n");

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
    //          DMP-NOT:           OMP
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
    //          PRT-AO-NEXT: //   #pragma omp distribute parallel for simd{{$}}
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
    //           PRT-O-NEXT:   {{^ *}}#pragma omp distribute parallel for simd{{$}}
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
  }

  //----------------------------------------------------------------------------
  // Check within accelerator routines.
  //----------------------------------------------------------------------------

  withinGangFnAccLoopSeqRun();
  withinGangFnAccLoopAutoGangRun();
  withinGangFnAccLoopAutoGangWorkerRun();
  withinGangFnAccLoopAutoGangVectorRun();
  withinGangFnAccLoopAutoGangWorkerVectorRun();
  withinGangFnAccLoopIndependentGangRun();
  withinGangFnAccLoopIndependentGangWorkerRun();
  withinGangFnAccLoopIndependentGangVectorRun();
  withinGangFnAccLoopIndependentGangWorkerVectorRun();

  withinWorkerFnAccLoopSeqRun();
  withinWorkerFnAccLoopAutoRun();
  withinWorkerFnAccLoopAutoWorkerRun();
  withinWorkerFnAccLoopAutoVectorRun();
  withinWorkerFnAccLoopAutoWorkerVectorRun();
  withinWorkerFnAccLoopIndependentRun();
  withinWorkerFnAccLoopIndependentWorkerRun();
  withinWorkerFnAccLoopIndependentVectorRun();
  withinWorkerFnAccLoopIndependentWorkerVectorRun();

  withinVectorFnAccLoopSeqRun();
  withinVectorFnAccLoopAutoRun();
  withinVectorFnAccLoopAutoVectorRun();
  withinVectorFnAccLoopIndependentRun();
  withinVectorFnAccLoopIndependentVectorRun();

  withinSeqFnAccLoopSeqRun();
  withinSeqFnAccLoopAutoRun();
  withinSeqFnAccLoopIndependentRun();

  return 0;
}

//==============================================================================
// Check within gang functions.
//
// This repeats the above testing but within a gang function and without
// statically enclosing parallel constructs.
//==============================================================================

//------------------------------------------------------------------------------
// Sequential loops.
//------------------------------------------------------------------------------

//..............................................................................
// acc loop seq
//..............................................................................

// DMP-LABEL: FunctionDecl {{.*}} withinGangFnAccLoopSeqRun
// PRT-LABEL: static void withinGangFnAccLoopSeqRun({{([^()]|[[:space:]])*}})
static void withinGangFnAccLoopSeqRun() {
  int loopOnly;
  int loopOnlyArr[1];
  struct Save save;

  //   EXE-NOT: {{.}}
  // EXE-LABEL: withinGangFn: acc loop seq{{$}}
  //   EXE-NOT: {{.}}
  memset(&save, 0, sizeof(save));
  loopOnly = 88;
  loopOnlyArr[0] = 88;
  printf("withinGangFn: acc loop seq\n");

  // Gang-redundant mode executes each loop iteration per gang.
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  #pragma acc parallel num_gangs(3)
  withinGangFnAccLoopSeq(&save, &loopOnly, loopOnlyArr);
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
}

// DMP-LABEL: FunctionDecl {{.*}} withinGangFnAccLoopSeq
// PRT-LABEL: static void withinGangFnAccLoopSeq({{([^()]|[[:space:]])*}})
static void withinGangFnAccLoopSeq(struct Save *save, int *loopOnly,
                                   int *loopOnlyArr) {
  // PRT: int j = 0;
  int j = 0;
  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCSeqClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  // DMP-NEXT:     DeclRefExpr {{.*}} 'j' 'int'
  // DMP-NEXT:     DeclRefExpr {{.*}} 'save'
  // DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnly' 'int *'
  // DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnlyArr' 'int *'
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
  #pragma acc loop seq
  for (int i = 0; i < 2; ++i) {
    TGT_PRINTF("loop iteration\n");
    for (j = 0; j < 2; ++j)
      ;
    checkLoopSeqPrivate(*save, i, loopOnly, *loopOnly, 88, 77);
    *loopOnly = 77;
    checkLoopSeqShared(*save, i, loopOnlyArr, 88, 77);
    loopOnlyArr[0] = 77;
  }
  // Each gang will see the final j from the j loop above.
  if (j != 2)
    save->declNestedLoopFoundBadVal = true;

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
  #pragma acc loop seq
  for (k = 1; k < 3; ++k) {
    TGT_PRINTF("loop iteration\n");
    for (l = 0; l < 2; ++l)
      ;
  }
  // Each gang will see the final k and l from the loop above.
  if (k != 3)
    save->assignLoopFoundBadVal = true;
  if (l != 2)
    save->assignNestedLoopFoundBadVal = true;
}

//..............................................................................
// acc loop auto GANG
//..............................................................................

// DMP-LABEL: FunctionDecl {{.*}} withinGangFnAccLoopAutoGangRun
// PRT-LABEL: static void withinGangFnAccLoopAutoGangRun({{([^()]|[[:space:]])*}})
static void withinGangFnAccLoopAutoGangRun() {
  int loopOnly;
  int loopOnlyArr[1];
  struct Save save;

  //   EXE-NOT: {{.}}
  // EXE-LABEL: withinGangFn: acc loop auto GANG{{$}}
  //   EXE-NOT: {{.}}
  memset(&save, 0, sizeof(save));
  loopOnly = 88;
  loopOnlyArr[0] = 88;
  printf("withinGangFn: acc loop auto GANG\n");

  // Gang-redundant mode executes each loop iteration per gang.
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  #pragma acc parallel num_gangs(3)
  withinGangFnAccLoopAutoGang(&save, &loopOnly, loopOnlyArr);
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
}

// DMP-LABEL: FunctionDecl {{.*}} withinGangFnAccLoopAutoGang
// PRT-LABEL: static void withinGangFnAccLoopAutoGang({{([^()]|[[:space:]])*}})
static void withinGangFnAccLoopAutoGang(struct Save *save, int *loopOnly,
                                        int *loopOnlyArr) {
  // PRT: int j = 0;
  int j = 0;
  //           DMP: ACCLoopDirective
  //      DMP-NEXT:   ACCAutoClause
  //       DMP-NOT:     <implicit>
  // DMP-GANG-NEXT:   ACCGangClause
  //  DMP-GANG-NOT:     <implicit>
  //      DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  //      DMP-NEXT:     DeclRefExpr {{.*}} 'j' 'int'
  //      DMP-NEXT:     DeclRefExpr {{.*}} 'save'
  //      DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnly' 'int *'
  //      DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnlyArr' 'int *'
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
  #pragma acc loop auto GANG
  for (int i = 0; i < 2; ++i) {
    TGT_PRINTF("loop iteration\n");
    for (j = 0; j < 2; ++j)
      ;
    checkLoopSeqPrivate(*save, i, loopOnly, *loopOnly, 88, 77);
    *loopOnly = 77;
    checkLoopSeqShared(*save, i, loopOnlyArr, 88, 77);
    loopOnlyArr[0] = 77;
  }
  // Each gang will see the final j from the j loop above.
  if (j != 2)
    save->declNestedLoopFoundBadVal = true;

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
  #pragma acc loop auto GANG
  for (k = 1; k < 3; ++k) {
    TGT_PRINTF("loop iteration\n");
    for (l = 0; l < 2; ++l)
      ;
  }
  // Each gang will see the old k from before the loop above.
  if (k != 999)
    save->assignLoopFoundBadVal = true;
  // Each gang will see the final l from the loop above.
  if (l != 2)
    save->assignNestedLoopFoundBadVal = true;

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
  #pragma acc loop auto GANG
  for (int i = 0; i < 2; ++i) {
    TGT_PRINTF("loop iteration\n");
    k = 9999;
  }
  // Each gang will see the k from the i loop above.
  if (k != 9999)
    save->writeInLaterLoopFoundBadVal = true;
}

//..............................................................................
// acc loop auto GANG worker
//..............................................................................

// DMP-LABEL: FunctionDecl {{.*}} withinGangFnAccLoopAutoGangWorkerRun
// PRT-LABEL: static void withinGangFnAccLoopAutoGangWorkerRun({{([^()]|[[:space:]])*}})
static void withinGangFnAccLoopAutoGangWorkerRun() {
  int loopOnly;
  int loopOnlyArr[1];
  struct Save save;

  //   EXE-NOT: {{.}}
  // EXE-LABEL: withinGangFn: acc loop auto GANG worker{{$}}
  //   EXE-NOT: {{.}}
  memset(&save, 0, sizeof(save));
  loopOnly = 88;
  loopOnlyArr[0] = 88;
  printf("withinGangFn: acc loop auto GANG worker\n");

  // Gang-redundant mode executes each loop iteration per gang.
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  #pragma acc parallel num_gangs(3)
  withinGangFnAccLoopAutoGangWorker(&save, &loopOnly, loopOnlyArr);
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
}

// DMP-LABEL: FunctionDecl {{.*}} withinGangFnAccLoopAutoGangWorker
// PRT-LABEL: static void withinGangFnAccLoopAutoGangWorker({{([^()]|[[:space:]])*}})
static void withinGangFnAccLoopAutoGangWorker(struct Save *save, int *loopOnly,
                                              int *loopOnlyArr) {
  // PRT: int j = 0;
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
  //      DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnly' 'int *'
  //      DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnlyArr' 'int *'
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
  //          PRT-OA-SAME: {{^ }}worker // discarded in OpenMP translation{{$}}
  //             PRT-NEXT: for ({{.*}}) {
  //             PRT-NEXT:   {{printf|TGT_PRINTF}}
  //             PRT-NEXT:   for (j ={{.*}})
  //             PRT-NEXT:     ;
  //                  PRT: }
  #pragma acc loop auto GANG worker
  for (int i = 0; i < 2; ++i) {
    TGT_PRINTF("loop iteration\n");
    for (j = 0; j < 2; ++j)
      ;
    checkLoopSeqPrivate(*save, i, loopOnly, *loopOnly, 88, 77);
    *loopOnly = 77;
    checkLoopSeqShared(*save, i, loopOnlyArr, 88, 77);
    loopOnlyArr[0] = 77;
  }
  // Each gang will see the final j from the j loop above.
  if (j != 2)
    save->declNestedLoopFoundBadVal = true;

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
  #pragma acc loop auto GANG worker
  for (k = 1; k < 3; ++k) {
    TGT_PRINTF("loop iteration\n");
    for (l = 0; l < 2; ++l)
      ;
  }
  // Each gang will see the old k from before the loop above.
  if (k != 999)
    save->assignLoopFoundBadVal = true;
  // Each gang will see the final l from the loop above.
  if (l != 2)
    save->assignNestedLoopFoundBadVal = true;

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
  //          PRT-OA-SAME: {{^ }}worker // discarded in OpenMP translation{{$}}
  //             PRT-NEXT: for (int i = {{.*}}) {
  //             PRT-NEXT:   {{printf|TGT_PRINTF}}
  //             PRT-NEXT:   k = 9999;
  //             PRT-NEXT: }
  #pragma acc loop auto GANG worker
  for (int i = 0; i < 2; ++i) {
    TGT_PRINTF("loop iteration\n");
    k = 9999;
  }
  // Each gang will see the k from the i loop above.
  if (k != 9999)
    save->writeInLaterLoopFoundBadVal = true;
}

//..............................................................................
// acc loop auto GANG vector
//..............................................................................

// DMP-LABEL: FunctionDecl {{.*}} withinGangFnAccLoopAutoGangVectorRun
// PRT-LABEL: static void withinGangFnAccLoopAutoGangVectorRun({{([^()]|[[:space:]])*}})
static void withinGangFnAccLoopAutoGangVectorRun() {
  int loopOnly;
  int loopOnlyArr[1];
  struct Save save;

  //   EXE-NOT: {{.}}
  // EXE-LABEL: withinGangFn: acc loop auto GANG vector{{$}}
  //   EXE-NOT: {{.}}
  memset(&save, 0, sizeof(save));
  loopOnly = 88;
  loopOnlyArr[0] = 88;
  printf("withinGangFn: acc loop auto GANG vector\n");

  // Gang-redundant mode executes each loop iteration per gang.
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  #pragma acc parallel num_gangs(3)
  withinGangFnAccLoopAutoGangVector(&save, &loopOnly, loopOnlyArr);
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
}

// DMP-LABEL: FunctionDecl {{.*}} withinGangFnAccLoopAutoGangVector
// PRT-LABEL: static void withinGangFnAccLoopAutoGangVector({{([^()]|[[:space:]])*}})
static void withinGangFnAccLoopAutoGangVector(struct Save *save, int *loopOnly,
                                              int *loopOnlyArr) {
  // PRT: int j = 0;
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
  //      DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnly' 'int *'
  //      DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnlyArr' 'int *'
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
  //          PRT-OA-SAME: {{^ }}vector // discarded in OpenMP translation{{$}}
  //             PRT-NEXT: for ({{.*}}) {
  //             PRT-NEXT:   {{printf|TGT_PRINTF}}
  //             PRT-NEXT:   for (j ={{.*}})
  //             PRT-NEXT:     ;
  //                  PRT: }
  #pragma acc loop auto GANG vector
  for (int i = 0; i < 2; ++i) {
    TGT_PRINTF("loop iteration\n");
    for (j = 0; j < 2; ++j)
      ;
    checkLoopSeqPrivate(*save, i, loopOnly, *loopOnly, 88, 77);
    *loopOnly = 77;
    checkLoopSeqShared(*save, i, loopOnlyArr, 88, 77);
    loopOnlyArr[0] = 77;
  }
  // Each gang will see the final j from the j loop above.
  if (j != 2)
    save->declNestedLoopFoundBadVal = true;

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
  #pragma acc loop auto GANG vector
  for (k = 1; k < 3; ++k) {
    TGT_PRINTF("loop iteration\n");
    for (l = 0; l < 2; ++l)
      ;
  }
  // Each gang will see the old k from before the loop above.
  if (k != 999)
    save->assignLoopFoundBadVal = true;
  // Each gang will see the final l from the loop above.
  if (l != 2)
    save->assignNestedLoopFoundBadVal = true;

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
  //          PRT-OA-SAME: {{^ }}vector // discarded in OpenMP translation{{$}}
  //             PRT-NEXT: for (int i = {{.*}}) {
  //             PRT-NEXT:   {{printf|TGT_PRINTF}}
  //             PRT-NEXT:   k = 9999;
  //             PRT-NEXT: }
  #pragma acc loop auto GANG vector
  for (int i = 0; i < 2; ++i) {
    TGT_PRINTF("loop iteration\n");
    k = 9999;
  }
  // Each gang will see the k from the i loop above.
  if (k != 9999)
    save->writeInLaterLoopFoundBadVal = true;
}

//..............................................................................
// acc loop auto GANG worker vector
//..............................................................................

// DMP-LABEL: FunctionDecl {{.*}} withinGangFnAccLoopAutoGangWorkerVectorRun
// PRT-LABEL: static void withinGangFnAccLoopAutoGangWorkerVectorRun({{([^()]|[[:space:]])*}})
static void withinGangFnAccLoopAutoGangWorkerVectorRun() {
  int loopOnly;
  int loopOnlyArr[1];
  struct Save save;

  //   EXE-NOT: {{.}}
  // EXE-LABEL: withinGangFn: acc loop auto GANG worker vector{{$}}
  //   EXE-NOT: {{.}}
  memset(&save, 0, sizeof(save));
  loopOnly = 88;
  loopOnlyArr[0] = 88;
  printf("withinGangFn: acc loop auto GANG worker vector\n");

  // Gang-redundant mode executes each loop iteration per gang.
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  #pragma acc parallel num_gangs(3)
  withinGangFnAccLoopAutoGangWorkerVector(&save, &loopOnly, loopOnlyArr);
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
}

// DMP-LABEL: FunctionDecl {{.*}} withinGangFnAccLoopAutoGangWorkerVector
// PRT-LABEL: static void withinGangFnAccLoopAutoGangWorkerVector({{([^()]|[[:space:]])*}})
static void withinGangFnAccLoopAutoGangWorkerVector(struct Save *save,
                                                    int *loopOnly,
                                                    int *loopOnlyArr) {
  // PRT: int j = 0;
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
  //      DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnly' 'int *'
  //      DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnlyArr' 'int *'
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
  //          PRT-OA-SAME: {{^ }}worker vector // discarded in OpenMP translation{{$}}
  //             PRT-NEXT: for ({{.*}}) {
  //             PRT-NEXT:   {{printf|TGT_PRINTF}}
  //             PRT-NEXT:   for (j ={{.*}})
  //             PRT-NEXT:     ;
  //                  PRT: }
  #pragma acc loop auto GANG worker vector
  for (int i = 0; i < 2; ++i) {
    TGT_PRINTF("loop iteration\n");
    for (j = 0; j < 2; ++j)
      ;
    checkLoopSeqPrivate(*save, i, loopOnly, *loopOnly, 88, 77);
    *loopOnly = 77;
    checkLoopSeqShared(*save, i, loopOnlyArr, 88, 77);
    loopOnlyArr[0] = 77;
  }
  // Each gang will see the final j from the j loop above.
  if (j != 2)
    save->declNestedLoopFoundBadVal = true;

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
  #pragma acc loop auto GANG worker vector
  for (k = 1; k < 3; ++k) {
    TGT_PRINTF("loop iteration\n");
    for (l = 0; l < 2; ++l)
      ;
  }
  // Each gang will see the old k from before the loop above.
  if (k != 999)
    save->assignLoopFoundBadVal = true;
  // Each gang will see the final l from the loop above.
  if (l != 2)
    save->assignNestedLoopFoundBadVal = true;

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
  //          PRT-OA-SAME: {{^ }}worker vector // discarded in OpenMP translation{{$}}
  //             PRT-NEXT: for (int i = {{.*}}) {
  //             PRT-NEXT:   {{printf|TGT_PRINTF}}
  //             PRT-NEXT:   k = 9999;
  //             PRT-NEXT: }
  #pragma acc loop auto GANG worker vector
  for (int i = 0; i < 2; ++i) {
    TGT_PRINTF("loop iteration\n");
    k = 9999;
  }
  // Each gang will see the k from the i loop above.
  if (k != 9999)
    save->writeInLaterLoopFoundBadVal = true;
}

//------------------------------------------------------------------------------
// acc loop INDEPENDENT GANG
//------------------------------------------------------------------------------

// DMP-LABEL: FunctionDecl {{.*}} withinGangFnAccLoopIndependentGangRun
// PRT-LABEL: static void withinGangFnAccLoopIndependentGangRun({{([^()]|[[:space:]])*}})
static void withinGangFnAccLoopIndependentGangRun() {
  int loopOnly;
  int loopOnlyArr[1];
  struct Save save;

  //   EXE-NOT: {{.}}
  // EXE-LABEL: withinGangFn: acc loop INDEPENDENT GANG{{$}}
  //   EXE-NOT: {{.}}
  memset(&save, 0, sizeof(save));
  loopOnly = 88;
  loopOnlyArr[0] = 88;
  printf("withinGangFn: acc loop INDEPENDENT GANG\n");

  // Gang-partitioned mode executes each loop iteration once.
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  #pragma acc parallel num_gangs(3)
  withinGangFnAccLoopIndependentGang(&save, &loopOnly, loopOnlyArr);

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

// DMP-LABEL: FunctionDecl {{.*}} withinGangFnAccLoopIndependentGang
// PRT-LABEL: static void withinGangFnAccLoopIndependentGang({{([^()]|[[:space:]])*}})
static void withinGangFnAccLoopIndependentGang(struct Save *save, int *loopOnly,
                                               int *loopOnlyArr) {
  // PRT: int j = 0;
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
  //         DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnly' 'int *'
  //         DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnlyArr' 'int *'
  // DMP-NO-GANG-NEXT:   ACCGangClause {{.*}} <implicit>
  //         DMP-NEXT:   impl: OMPDistributeDirective
  //          DMP-NOT:     OMP
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
  #pragma acc loop INDEPENDENT GANG
  for (int i = 0; i < 2; ++i) {
    TGT_PRINTF("loop iteration\n");
    for (j = 0; j < 2; ++j)
      ;
    checkLoopPartPrivate(*save, i, loopOnly, *loopOnly, 88, 77);
    *loopOnly = 77;
    checkLoopPartShared(*save, i, loopOnlyArr, loopOnlyArr[0], 88, 77);
    loopOnlyArr[0] = 77;
  }
  // Each gang that is assigned one or more i iterations will see the final
  // j from the j loop above, and the remaining gangs will see the old j
  // value.
  if (j == 0)
    save->declNestedLoopFoundOldVal = true;
  else if (j == 2)
    save->declNestedLoopFoundNewVal = true;
  else
    save->declNestedLoopFoundBadVal = true;

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
  //          DMP-NOT:     OMP
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
  #pragma acc loop INDEPENDENT GANG
  for (k = 1; k < 3; ++k) {
    TGT_PRINTF("loop iteration\n");
    for (l = 0; l < 2; ++l)
      ;
  }
  // Each gang will see the old k from before the loop above.
  if (k != 999)
    save->assignLoopFoundBadVal = true;
  // Each gang that is assigned one or more k iterations will see the final
  // l from the l loop above, and the remaining gangs will see the old l
  // value.
  if (l == 888)
    save->assignNestedLoopFoundOldVal = true;
  else if (l == 2)
    save->assignNestedLoopFoundNewVal = true;
  else
    save->assignNestedLoopFoundBadVal = true;

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
  //          DMP-NOT:     OMP
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
  #pragma acc loop INDEPENDENT GANG
  for (int i = 0; i < 2; ++i) {
    TGT_PRINTF("loop iteration\n");
    k = 9999;
  }
  // Each gang that is assigned one or more i iterations will see the k from
  // the i loop above, and the remaining gangs will see the old k value.
  if (k == 999)
    save->writeInLaterLoopFoundOldVal = true;
  else if (k == 9999)
    save->writeInLaterLoopFoundNewVal = true;
  else
    save->writeInLaterLoopFoundBadVal = true;
}

//------------------------------------------------------------------------------
// acc loop INDEPENDENT GANG worker
//------------------------------------------------------------------------------

// DMP-LABEL: FunctionDecl {{.*}} withinGangFnAccLoopIndependentGangWorkerRun
// PRT-LABEL: static void withinGangFnAccLoopIndependentGangWorkerRun({{([^()]|[[:space:]])*}})
static void withinGangFnAccLoopIndependentGangWorkerRun() {
  int loopOnly;
  int loopOnlyArr[1];
  struct Save save;

  //   EXE-NOT: {{.}}
  // EXE-LABEL: withinGangFn: acc loop INDEPENDENT GANG worker{{$}}
  //   EXE-NOT: {{.}}
  memset(&save, 0, sizeof(save));
  loopOnly = 88;
  loopOnlyArr[0] = 88;
  printf("withinGangFn: acc loop INDEPENDENT GANG worker\n");

  // Gang-partitioned mode executes each loop iteration once.
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  #pragma acc parallel num_gangs(3)
  withinGangFnAccLoopIndependentGangWorker(&save, &loopOnly, loopOnlyArr);

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

// DMP-LABEL: FunctionDecl {{.*}} withinGangFnAccLoopIndependentGangWorker
// PRT-LABEL: static void withinGangFnAccLoopIndependentGangWorker({{([^()]|[[:space:]])*}})
static void withinGangFnAccLoopIndependentGangWorker(struct Save *save,
                                                     int *loopOnly,
                                                     int *loopOnlyArr) {
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
  //         DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnly' 'int *'
  //         DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnlyArr' 'int *'
  // DMP-NO-GANG-NEXT:   ACCGangClause {{.*}} <implicit>
  //         DMP-NEXT:   impl: OMPDistributeParallelForDirective
  //          DMP-NOT:     OMP
  //              DMP:     ForStmt
  //
  //           PRT-A-NEXT: {{^ *}}#pragma acc loop
  //       PRT-A-SRC-SAME: {{^ }}INDEPENDENT GANG
  //   PRT-A-AST-IND-SAME: {{^ }}independent
  //  PRT-A-AST-GANG-SAME: {{^ }}gang
  //           PRT-A-SAME: {{^ }}worker{{$}}
  //          PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for{{$}}
  //           PRT-O-NEXT: {{^ *}}#pragma omp distribute parallel for{{$}}
  //          PRT-OA-NEXT: {{^ *}}// #pragma acc loop
  //      PRT-OA-SRC-SAME: {{^ }}INDEPENDENT GANG
  //  PRT-OA-AST-IND-SAME: {{^ }}independent
  // PRT-OA-AST-GANG-SAME: {{^ }}gang
  //          PRT-OA-SAME: {{^ }}worker{{$}}
  //             PRT-NEXT: for (int i = {{.*}}) {
  //             PRT-NEXT:   {{printf|TGT_PRINTF}}
  //                  PRT: }
  #pragma acc loop INDEPENDENT GANG worker
  for (int i = 0; i < 2; ++i) {
    TGT_PRINTF("loop iteration\n");
    // We omit checking of an inner j loop this time.  j would be shared
    // among workers and thus would have an unpredictable value.
    checkLoopPartShared(*save, i, loopOnly, *loopOnly, 88, 77);
    *loopOnly = 77;
    checkLoopPartShared(*save, i, loopOnlyArr, loopOnlyArr[0], 88, 77);
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
  #pragma acc loop INDEPENDENT GANG worker
  for (k = 1; k < 3; ++k) {
    TGT_PRINTF("loop iteration\n");
    // We omit checking of an inner l loop this time.  l would be shared
    // among workers and thus would have an unpredictable value.
  }
  // Each gang will see the old k from before the loop above.
  if (k != 999)
    save->assignLoopFoundBadVal = true;

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
  //          DMP-NOT:     OMP
  //              DMP:     ForStmt
  //
  //           PRT-A-NEXT: {{^ *}}#pragma acc loop
  //       PRT-A-SRC-SAME: {{^ }}INDEPENDENT GANG
  //   PRT-A-AST-IND-SAME: {{^ }}independent
  //  PRT-A-AST-GANG-SAME: {{^ }}gang
  //           PRT-A-SAME: {{^ }}worker{{$}}
  //          PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for{{$}}
  //           PRT-O-NEXT: {{^ *}}#pragma omp distribute parallel for{{$}}
  //          PRT-OA-NEXT: {{^ *}}// #pragma acc loop
  //      PRT-OA-SRC-SAME: {{^ }}INDEPENDENT GANG
  //  PRT-OA-AST-IND-SAME: {{^ }}independent
  // PRT-OA-AST-GANG-SAME: {{^ }}gang
  //          PRT-OA-SAME: {{^ }}worker{{$}}
  //             PRT-NEXT: for ({{.*}}) {
  //             PRT-NEXT:   {{printf|TGT_PRINTF}}
  //             PRT-NEXT:   k = 9999;
  //             PRT-NEXT: }
  #pragma acc loop INDEPENDENT GANG worker
  for (int i = 0; i < 2; ++i) {
    TGT_PRINTF("loop iteration\n");
    k = 9999;
  }
  // Each gang that is assigned one or more i iterations will see the k from
  // the i loop above, and the remaining gangs will see the old k value.
  if (k == 999)
    save->writeInLaterLoopFoundOldVal = true;
  else if (k == 9999)
    save->writeInLaterLoopFoundNewVal = true;
  else
    save->writeInLaterLoopFoundBadVal = true;
}

//------------------------------------------------------------------------------
// acc loop INDEPENDENT GANG vector
//------------------------------------------------------------------------------

// DMP-LABEL: FunctionDecl {{.*}} withinGangFnAccLoopIndependentGangVectorRun
// PRT-LABEL: static void withinGangFnAccLoopIndependentGangVectorRun({{([^()]|[[:space:]])*}})
static void withinGangFnAccLoopIndependentGangVectorRun() {
  int loopOnly;
  int loopOnlyArr[1];
  struct Save save;

  //   EXE-NOT: {{.}}
  // EXE-LABEL: withinGangFn: acc loop INDEPENDENT GANG vector{{$}}
  //   EXE-NOT: {{.}}
  memset(&save, 0, sizeof(save));
  loopOnly = 88;
  loopOnlyArr[0] = 88;
  printf("withinGangFn: acc loop INDEPENDENT GANG vector\n");

  // Gang-partitioned mode executes each loop iteration once.
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  #pragma acc parallel num_gangs(3)
  withinGangFnAccLoopIndependentGangVector(&save, &loopOnly, loopOnlyArr);

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

// DMP-LABEL: FunctionDecl {{.*}} withinGangFnAccLoopIndependentGangVector
// PRT-LABEL: static void withinGangFnAccLoopIndependentGangVector({{([^()]|[[:space:]])*}})
static void withinGangFnAccLoopIndependentGangVector(struct Save *save,
                                                     int *loopOnly,
                                                     int *loopOnlyArr) {
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
  //         DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnly' 'int *'
  //         DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnlyArr' 'int *'
  // DMP-NO-GANG-NEXT:   ACCGangClause {{.*}} <implicit>
  //         DMP-NEXT:   impl: OMPDistributeSimdDirective
  //          DMP-NOT:     OMP
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
  #pragma acc loop INDEPENDENT GANG vector
  for (int i = 0; i < 2; ++i) {
    TGT_PRINTF("loop iteration\n");
    // We omit checking of an inner j loop this time.  j would be shared
    // among vector lanes and thus would have an unpredictable value.
    checkLoopPartShared(*save, i, loopOnly, *loopOnly, 88, 77);
    *loopOnly = 77;
    checkLoopPartShared(*save, i, loopOnlyArr, loopOnlyArr[0], 88, 77);
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
  #pragma acc loop INDEPENDENT GANG vector
  for (k = 1; k < 3; ++k) {
    TGT_PRINTF("loop iteration\n");
    // We omit checking of an inner l loop this time.  l would be shared
    // among vector lanes and thus would have an unpredictable value.
  }
  // Each gang will see the old k from before the loop above.
  if (k != 999)
    save->assignLoopFoundBadVal = true;

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
  //          DMP-NOT:     OMP
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
  #pragma acc loop INDEPENDENT GANG vector
  for (int i = 0; i < 2; ++i) {
    TGT_PRINTF("loop iteration\n");
    k = 9999;
  }
  // Each gang that is assigned one or more i iterations will see the k from
  // the i loop above, and the remaining gangs will see the old k value.
  if (k == 999)
    save->writeInLaterLoopFoundOldVal = true;
  else if (k == 9999)
    save->writeInLaterLoopFoundNewVal = true;
  else
    save->writeInLaterLoopFoundBadVal = true;
}

//------------------------------------------------------------------------------
// acc loop INDEPENDENT GANG worker vector
//------------------------------------------------------------------------------

// DMP-LABEL: FunctionDecl {{.*}} withinGangFnAccLoopIndependentGangWorkerVectorRun
// PRT-LABEL: static void withinGangFnAccLoopIndependentGangWorkerVectorRun({{([^()]|[[:space:]])*}})
static void withinGangFnAccLoopIndependentGangWorkerVectorRun() {
  int loopOnly;
  int loopOnlyArr[1];
  struct Save save;

  //   EXE-NOT: {{.}}
  // EXE-LABEL: withinGangFn: acc loop INDEPENDENT GANG worker vector{{$}}
  //   EXE-NOT: {{.}}
  memset(&save, 0, sizeof(save));
  loopOnly = 88;
  loopOnlyArr[0] = 88;
  printf("withinGangFn: acc loop INDEPENDENT GANG worker vector\n");

  // Gang-partitioned mode executes each loop iteration once.
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  #pragma acc parallel num_gangs(3)
  withinGangFnAccLoopIndependentGangWorkerVector(&save, &loopOnly, loopOnlyArr);

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

// DMP-LABEL: FunctionDecl {{.*}} withinGangFnAccLoopIndependentGangWorkerVector
// PRT-LABEL: static void withinGangFnAccLoopIndependentGangWorkerVector({{([^()]|[[:space:]])*}})
static void withinGangFnAccLoopIndependentGangWorkerVector(struct Save *save,
                                                          int *loopOnly,
                                                          int *loopOnlyArr) {
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
  //         DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnly' 'int *'
  //         DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnlyArr' 'int *'
  // DMP-NO-GANG-NEXT:   ACCGangClause {{.*}} <implicit>
  //         DMP-NEXT:   impl: OMPDistributeParallelForSimdDirective
  //          DMP-NOT:     OMP
  //              DMP:     ForStmt
  //
  //           PRT-A-NEXT: {{^ *}}#pragma acc loop
  //       PRT-A-SRC-SAME: {{^ }}INDEPENDENT GANG
  //   PRT-A-AST-IND-SAME: {{^ }}independent
  //  PRT-A-AST-GANG-SAME: {{^ }}gang
  //           PRT-A-SAME: {{^ }}worker vector{{$}}
  //          PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for simd{{$}}
  //           PRT-O-NEXT: {{^ *}}#pragma omp distribute parallel for simd{{$}}
  //          PRT-OA-NEXT: {{^ *}}// #pragma acc loop
  //      PRT-OA-SRC-SAME: {{^ }}INDEPENDENT GANG
  //  PRT-OA-AST-IND-SAME: {{^ }}independent
  // PRT-OA-AST-GANG-SAME: {{^ }}gang
  //          PRT-OA-SAME: {{^ }}worker vector{{$}}
  //             PRT-NEXT: for (int i = {{.*}}) {
  //             PRT-NEXT:   {{printf|TGT_PRINTF}}
  //                  PRT: }
  #pragma acc loop INDEPENDENT GANG worker vector
  for (int i = 0; i < 2; ++i) {
    TGT_PRINTF("loop iteration\n");
    // We omit checking of an inner j loop this time.  j would be shared
    // among workers and vector lanes and thus would have an unpredictable
    // value.
    checkLoopPartShared(*save, i, loopOnly, *loopOnly, 88, 77);
    *loopOnly = 77;
    checkLoopPartShared(*save, i, loopOnlyArr, loopOnlyArr[0], 88, 77);
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
  #pragma acc loop INDEPENDENT GANG worker vector
  for (k = 1; k < 3; ++k) {
    TGT_PRINTF("loop iteration\n");
    // We omit checking of an inner l loop this time.  l would be shared
    // among workers and vector lanes and thus would have an unpredictable
    // value.
  }
  // Each gang will see the old k from before the loop above.
  if (k != 999)
    save->assignLoopFoundBadVal = true;

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
  //          DMP-NOT:     OMP
  //              DMP:     ForStmt
  //
  //           PRT-A-NEXT: {{^ *}}#pragma acc loop
  //       PRT-A-SRC-SAME: {{^ }}INDEPENDENT GANG
  //   PRT-A-AST-IND-SAME: {{^ }}independent
  //  PRT-A-AST-GANG-SAME: {{^ }}gang
  //           PRT-A-SAME: {{^ }}worker vector{{$}}
  //          PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for simd{{$}}
  //           PRT-O-NEXT: {{^ *}}#pragma omp distribute parallel for simd{{$}}
  //          PRT-OA-NEXT: {{^ *}}// #pragma acc loop
  //      PRT-OA-SRC-SAME: {{^ }}INDEPENDENT GANG
  //  PRT-OA-AST-IND-SAME: {{^ }}independent
  // PRT-OA-AST-GANG-SAME: {{^ }}gang
  //          PRT-OA-SAME: {{^ }}worker vector{{$}}
  //             PRT-NEXT: for ({{.*}}) {
  //             PRT-NEXT:   {{printf|TGT_PRINTF}}
  //             PRT-NEXT:   k = 9999;
  //             PRT-NEXT: }
  #pragma acc loop INDEPENDENT GANG worker vector
  for (int i = 0; i < 2; ++i) {
    TGT_PRINTF("loop iteration\n");
    k = 9999;
  }
  // Each gang that is assigned one or more i iterations will see the k from
  // the i loop above, and the remaining gangs will see the old k value.
  if (k == 999)
    save->writeInLaterLoopFoundOldVal = true;
  else if (k == 9999)
    save->writeInLaterLoopFoundNewVal = true;
  else
    save->writeInLaterLoopFoundBadVal = true;
}

//==============================================================================
// Check within worker functions.
//
// This repeats the above testing but within a worker function.
//
// In all cases, check calling worker functions directly from the enclosing
// parallel construct.  For sequential loops within worker functions, that
// should be sufficient to prove the loops generally execute sequentially.  For
// partitioned loops within worker functions, also check calling the worker
// functions from gang loops to make sure nesting parallelism levels in that
// way works correctly at run time.
//==============================================================================

//------------------------------------------------------------------------------
// Sequential loops.
//------------------------------------------------------------------------------

//..............................................................................
// acc loop seq
//..............................................................................

// DMP-LABEL: FunctionDecl {{.*}} withinWorkerFnAccLoopSeqRun
// PRT-LABEL: static void withinWorkerFnAccLoopSeqRun({{([^()]|[[:space:]])*}})
static void withinWorkerFnAccLoopSeqRun() {
  int loopOnly;
  int loopOnlyArr[1];
  struct Save save;

  //   EXE-NOT: {{.}}
  // EXE-LABEL: withinWorkerFn: acc loop seq{{$}}
  //   EXE-NOT: {{.}}
  memset(&save, 0, sizeof(save));
  loopOnly = 88;
  loopOnlyArr[0] = 88;
  printf("withinWorkerFn: acc loop seq\n");

  // Gang-redundant mode executes each loop iteration per gang.
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  #pragma acc parallel num_gangs(3)
  withinWorkerFnAccLoopSeq(&save, &loopOnly, loopOnlyArr);
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
}

// DMP-LABEL: FunctionDecl {{.*}} withinWorkerFnAccLoopSeq
// PRT-LABEL: static void withinWorkerFnAccLoopSeq({{([^()]|[[:space:]])*}})
static void withinWorkerFnAccLoopSeq(struct Save *save, int *loopOnly,
                                     int *loopOnlyArr) {
  // PRT: int j = 0;
  int j = 0;
  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCSeqClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  // DMP-NEXT:     DeclRefExpr {{.*}} 'j' 'int'
  // DMP-NEXT:     DeclRefExpr {{.*}} 'save'
  // DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnly' 'int *'
  // DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnlyArr' 'int *'
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
  #pragma acc loop seq
  for (int i = 0; i < 2; ++i) {
    TGT_PRINTF("loop iteration\n");
    for (j = 0; j < 2; ++j)
      ;
    checkLoopSeqPrivate(*save, i, loopOnly, *loopOnly, 88, 77);
    *loopOnly = 77;
    checkLoopSeqShared(*save, i, loopOnlyArr, 88, 77);
    loopOnlyArr[0] = 77;
  }
  // Each gang will see the final j from the j loop above.
  if (j != 2)
    save->declNestedLoopFoundBadVal = true;

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
  #pragma acc loop seq
  for (k = 1; k < 3; ++k) {
    TGT_PRINTF("loop iteration\n");
    for (l = 0; l < 2; ++l)
      ;
  }
  // Each gang will see the final k and l from the loop above.
  if (k != 3)
    save->assignLoopFoundBadVal = true;
  if (l != 2)
    save->assignNestedLoopFoundBadVal = true;
}

//..............................................................................
// acc loop auto
//..............................................................................

// DMP-LABEL: FunctionDecl {{.*}} withinWorkerFnAccLoopAutoRun
// PRT-LABEL: static void withinWorkerFnAccLoopAutoRun({{([^()]|[[:space:]])*}})
static void withinWorkerFnAccLoopAutoRun() {
  int loopOnly;
  int loopOnlyArr[1];
  struct Save save;

  //   EXE-NOT: {{.}}
  // EXE-LABEL: withinWorkerFn: acc loop auto{{$}}
  //   EXE-NOT: {{.}}
  memset(&save, 0, sizeof(save));
  loopOnly = 88;
  loopOnlyArr[0] = 88;
  printf("withinWorkerFn: acc loop auto\n");

  // Gang-redundant mode executes each loop iteration per gang.
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  #pragma acc parallel num_gangs(3)
  withinWorkerFnAccLoopAuto(&save, &loopOnly, loopOnlyArr);

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
}

// DMP-LABEL: FunctionDecl {{.*}} withinWorkerFnAccLoopAuto
// PRT-LABEL: static void withinWorkerFnAccLoopAuto({{([^()]|[[:space:]])*}})
static void withinWorkerFnAccLoopAuto(struct Save *save, int *loopOnly,
                                      int *loopOnlyArr) {
  // PRT: int j = 0;
  int j = 0;
  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCAutoClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  // DMP-NEXT:     DeclRefExpr {{.*}} 'j' 'int'
  // DMP-NEXT:     DeclRefExpr {{.*}} 'save'
  // DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnly' 'int *'
  // DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnlyArr' 'int *'
  // DMP-NEXT:   impl: ForStmt
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop auto
  // PRT-AO-SAME: {{^ }}// discarded in OpenMP translation
  //  PRT-A-SAME: {{^$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop auto // discarded in OpenMP translation{{$}}
  //    PRT-NEXT: for ({{.*}}) {
  //    PRT-NEXT:   {{printf|TGT_PRINTF}}
  //    PRT-NEXT:   for (j ={{.*}})
  //    PRT-NEXT:     ;
  //         PRT: }
  #pragma acc loop auto
  for (int i = 0; i < 2; ++i) {
    TGT_PRINTF("loop iteration\n");
    for (j = 0; j < 2; ++j)
      ;
    checkLoopSeqPrivate(*save, i, loopOnly, *loopOnly, 88, 77);
    *loopOnly = 77;
    checkLoopSeqShared(*save, i, loopOnlyArr, 88, 77);
    loopOnlyArr[0] = 77;
  }
  // Each gang will see the final j from the j loop above.
  if (j != 2)
    save->declNestedLoopFoundBadVal = true;

  //      PRT: int k = 999;
  // PRT-NEXT: int l = 888;
  int k = 999;
  int l = 888;
  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCAutoClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCPrivateClause {{.*}} <predetermined>
  // DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
  // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  // DMP-NEXT:     DeclRefExpr {{.*}} 'l' 'int'
  // DMP-NEXT:   impl: CompoundStmt
  // DMP-NEXT:     DeclStmt
  // DMP-NEXT:       VarDecl {{.*}} k 'int'
  // DMP-NEXT:     ForStmt
  //
  // PRT-AO-NEXT: // v----------ACC----------v
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop auto{{$}}
  //  PRT-A-NEXT: for (k ={{.*}}) {
  //  PRT-A-NEXT:   {{print|TGT_PRINTF}}
  //  PRT-A-NEXT:   for (l ={{.*}})
  //  PRT-A-NEXT:     ;
  //  PRT-A-NEXT: }
  // PRT-AO-NEXT: // ---------ACC->OMP--------
  // PRT-AO-NEXT: // {
  // PRT-AO-NEXT: //   int k;
  // PRT-AO-NEXT: //   for (k ={{.*}}) {
  // PRT-AO-NEXT: //     {{printf|TGT_PRINTF}}
  // PRT-AO-NEXT: //     for (l ={{.*}})
  // PRT-AO-NEXT: //       ;
  // PRT-AO-NEXT: //   }
  // PRT-AO-NEXT: // }
  // PRT-AO-NEXT: // ^----------OMP----------^
  //
  // PRT-OA-NEXT: // v----------OMP----------v
  //  PRT-O-NEXT: {
  //  PRT-O-NEXT:   int k;
  //  PRT-O-NEXT:   for (k ={{.*}}) {
  //  PRT-O-NEXT:     {{printf|TGT_PRINTF}}
  //  PRT-O-NEXT:     for (l ={{.*}})
  //  PRT-O-NEXT:       ;
  //  PRT-O-NEXT:   }
  //  PRT-O-NEXT: }
  // PRT-OA-NEXT: // ---------OMP<-ACC--------
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop auto{{$}}
  // PRT-OA-NEXT: // for (k ={{.*}}) {
  // PRT-OA-NEXT: //   {{printf|TGT_PRINTF}}
  // PRT-OA-NEXT: //   for (l ={{.*}})
  // PRT-OA-NEXT: //     ;
  // PRT-OA-NEXT: // }
  // PRT-OA-NEXT: // ^----------ACC----------^
  //
  // PRT-NOACC-NEXT: for (k ={{.*}}) {
  // PRT-NOACC-NEXT:   {{printf|TGT_PRINTF}}
  // PRT-NOACC-NEXT:   for (l ={{.*}})
  // PRT-NOACC-NEXT:     ;
  // PRT-NOACC-NEXT: }
  #pragma acc loop auto
  for (k = 1; k < 3; ++k) {
    TGT_PRINTF("loop iteration\n");
    for (l = 0; l < 2; ++l)
      ;
  }
  // Each gang will see the old k from before the loop above.
  if (k != 999)
    save->assignLoopFoundBadVal = true;
  // Each gang will see the final l from the loop above.
  if (l != 2)
    save->assignNestedLoopFoundBadVal = true;

  // PRT: k = 999;
  k = 999;
  // k should have been dropped from the loop control variable list, so it
  // should now be implicitly shared in all cases.
  //
  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCAutoClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  // DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
  // DMP-NEXT:   impl: ForStmt
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop auto
  // PRT-AO-SAME: {{^ }}// discarded in OpenMP translation
  //  PRT-A-SAME: {{^$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop auto // discarded in OpenMP translation{{$}}
  //    PRT-NEXT: for (int i = {{.*}}) {
  //    PRT-NEXT:   {{printf|TGT_PRINTF}}
  //    PRT-NEXT:   k = 9999;
  //    PRT-NEXT: }
  #pragma acc loop auto
  for (int i = 0; i < 2; ++i) {
    TGT_PRINTF("loop iteration\n");
    k = 9999;
  }
  // Each gang will see the k from the i loop above.
  if (k != 9999)
    save->writeInLaterLoopFoundBadVal = true;
}

//..............................................................................
// acc loop auto worker
//..............................................................................

// DMP-LABEL: FunctionDecl {{.*}} withinWorkerFnAccLoopAutoWorkerRun
// PRT-LABEL: static void withinWorkerFnAccLoopAutoWorkerRun({{([^()]|[[:space:]])*}})
static void withinWorkerFnAccLoopAutoWorkerRun() {
  int loopOnly;
  int loopOnlyArr[1];
  struct Save save;

  //   EXE-NOT: {{.}}
  // EXE-LABEL: withinWorkerFn: acc loop auto worker{{$}}
  //   EXE-NOT: {{.}}
  memset(&save, 0, sizeof(save));
  loopOnly = 88;
  loopOnlyArr[0] = 88;
  printf("withinWorkerFn: acc loop auto worker\n");

  // Gang-redundant mode executes each loop iteration per gang.
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  #pragma acc parallel num_gangs(3)
  withinWorkerFnAccLoopAutoWorker(&save, &loopOnly, loopOnlyArr);

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
}

// DMP-LABEL: FunctionDecl {{.*}} withinWorkerFnAccLoopAutoWorker
// PRT-LABEL: static void withinWorkerFnAccLoopAutoWorker({{([^()]|[[:space:]])*}})
static void withinWorkerFnAccLoopAutoWorker(struct Save *save, int *loopOnly,
                                            int *loopOnlyArr) {
  // PRT: int j = 0;
  int j = 0;
  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCAutoClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCWorkerClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  // DMP-NEXT:     DeclRefExpr {{.*}} 'j' 'int'
  // DMP-NEXT:     DeclRefExpr {{.*}} 'save'
  // DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnly' 'int *'
  // DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnlyArr' 'int *'
  // DMP-NEXT:   impl: ForStmt
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop auto worker
  // PRT-AO-SAME: {{^ }}// discarded in OpenMP translation
  //  PRT-A-SAME: {{^$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop auto worker // discarded in OpenMP translation{{$}}
  //    PRT-NEXT: for ({{.*}}) {
  //    PRT-NEXT:   {{printf|TGT_PRINTF}}
  //    PRT-NEXT:   for (j ={{.*}})
  //    PRT-NEXT:     ;
  //         PRT: }
  #pragma acc loop auto worker
  for (int i = 0; i < 2; ++i) {
    TGT_PRINTF("loop iteration\n");
    for (j = 0; j < 2; ++j)
      ;
    checkLoopSeqPrivate(*save, i, loopOnly, *loopOnly, 88, 77);
    *loopOnly = 77;
    checkLoopSeqShared(*save, i, loopOnlyArr, 88, 77);
    loopOnlyArr[0] = 77;
  }
  // Each gang will see the final j from the j loop above.
  if (j != 2)
    save->declNestedLoopFoundBadVal = true;

  //      PRT: int k = 999;
  // PRT-NEXT: int l = 888;
  int k = 999;
  int l = 888;
  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCAutoClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCWorkerClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCPrivateClause {{.*}} <predetermined>
  // DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
  // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  // DMP-NEXT:     DeclRefExpr {{.*}} 'l' 'int'
  // DMP-NEXT:   impl: CompoundStmt
  // DMP-NEXT:     DeclStmt
  // DMP-NEXT:       VarDecl {{.*}} k 'int'
  // DMP-NEXT:     ForStmt
  //
  // PRT-AO-NEXT: // v----------ACC----------v
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop auto worker{{$}}
  //  PRT-A-NEXT: for (k ={{.*}}) {
  //  PRT-A-NEXT:   {{print|TGT_PRINTF}}
  //  PRT-A-NEXT:   for (l ={{.*}})
  //  PRT-A-NEXT:     ;
  //  PRT-A-NEXT: }
  // PRT-AO-NEXT: // ---------ACC->OMP--------
  // PRT-AO-NEXT: // {
  // PRT-AO-NEXT: //   int k;
  // PRT-AO-NEXT: //   for (k ={{.*}}) {
  // PRT-AO-NEXT: //     {{printf|TGT_PRINTF}}
  // PRT-AO-NEXT: //     for (l ={{.*}})
  // PRT-AO-NEXT: //       ;
  // PRT-AO-NEXT: //   }
  // PRT-AO-NEXT: // }
  // PRT-AO-NEXT: // ^----------OMP----------^
  //
  // PRT-OA-NEXT: // v----------OMP----------v
  //  PRT-O-NEXT: {
  //  PRT-O-NEXT:   int k;
  //  PRT-O-NEXT:   for (k ={{.*}}) {
  //  PRT-O-NEXT:     {{printf|TGT_PRINTF}}
  //  PRT-O-NEXT:     for (l ={{.*}})
  //  PRT-O-NEXT:       ;
  //  PRT-O-NEXT:   }
  //  PRT-O-NEXT: }
  // PRT-OA-NEXT: // ---------OMP<-ACC--------
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop auto worker{{$}}
  // PRT-OA-NEXT: // for (k ={{.*}}) {
  // PRT-OA-NEXT: //   {{printf|TGT_PRINTF}}
  // PRT-OA-NEXT: //   for (l ={{.*}})
  // PRT-OA-NEXT: //     ;
  // PRT-OA-NEXT: // }
  // PRT-OA-NEXT: // ^----------ACC----------^
  //
  // PRT-NOACC-NEXT: for (k ={{.*}}) {
  // PRT-NOACC-NEXT:   {{printf|TGT_PRINTF}}
  // PRT-NOACC-NEXT:   for (l ={{.*}})
  // PRT-NOACC-NEXT:     ;
  // PRT-NOACC-NEXT: }
  #pragma acc loop auto worker
  for (k = 1; k < 3; ++k) {
    TGT_PRINTF("loop iteration\n");
    for (l = 0; l < 2; ++l)
      ;
  }
  // Each gang will see the old k from before the loop above.
  if (k != 999)
    save->assignLoopFoundBadVal = true;
  // Each gang will see the final l from the loop above.
  if (l != 2)
    save->assignNestedLoopFoundBadVal = true;

  // PRT: k = 999;
  k = 999;
  // k should have been dropped from the loop control variable list, so it
  // should now be implicitly shared in all cases.
  //
  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCAutoClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCWorkerClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  // DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
  // DMP-NEXT:   impl: ForStmt
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop auto worker
  // PRT-AO-SAME: {{^ }}// discarded in OpenMP translation
  //  PRT-A-SAME: {{^$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop auto worker // discarded in OpenMP translation{{$}}
  //    PRT-NEXT: for (int i = {{.*}}) {
  //    PRT-NEXT:   {{printf|TGT_PRINTF}}
  //    PRT-NEXT:   k = 9999;
  //    PRT-NEXT: }
  #pragma acc loop auto worker
  for (int i = 0; i < 2; ++i) {
    TGT_PRINTF("loop iteration\n");
    k = 9999;
  }
  // Each gang will see the k from the i loop above.
  if (k != 9999)
    save->writeInLaterLoopFoundBadVal = true;
}

//..............................................................................
// acc loop auto vector
//..............................................................................

// DMP-LABEL: FunctionDecl {{.*}} withinWorkerFnAccLoopAutoVectorRun
// PRT-LABEL: static void withinWorkerFnAccLoopAutoVectorRun({{([^()]|[[:space:]])*}})
static void withinWorkerFnAccLoopAutoVectorRun() {
  int loopOnly;
  int loopOnlyArr[1];
  struct Save save;

  //   EXE-NOT: {{.}}
  // EXE-LABEL: withinWorkerFn: acc loop auto vector{{$}}
  //   EXE-NOT: {{.}}
  memset(&save, 0, sizeof(save));
  loopOnly = 88;
  loopOnlyArr[0] = 88;
  printf("withinWorkerFn: acc loop auto vector\n");

  // Gang-redundant mode executes each loop iteration per gang.
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  #pragma acc parallel num_gangs(3)
  withinWorkerFnAccLoopAutoVector(&save, &loopOnly, loopOnlyArr);

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
}

// DMP-LABEL: FunctionDecl {{.*}} withinWorkerFnAccLoopAutoVector
// PRT-LABEL: static void withinWorkerFnAccLoopAutoVector({{([^()]|[[:space:]])*}})
static void withinWorkerFnAccLoopAutoVector(struct Save *save, int *loopOnly,
                                            int *loopOnlyArr) {
  // PRT: int j = 0;
  int j = 0;
  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCAutoClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCVectorClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  // DMP-NEXT:     DeclRefExpr {{.*}} 'j' 'int'
  // DMP-NEXT:     DeclRefExpr {{.*}} 'save'
  // DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnly' 'int *'
  // DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnlyArr' 'int *'
  // DMP-NEXT:   impl: ForStmt
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop auto vector
  // PRT-AO-SAME: {{^ }}// discarded in OpenMP translation
  //  PRT-A-SAME: {{^$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop auto vector // discarded in OpenMP translation{{$}}
  //    PRT-NEXT: for ({{.*}}) {
  //    PRT-NEXT:   {{printf|TGT_PRINTF}}
  //    PRT-NEXT:   for (j ={{.*}})
  //    PRT-NEXT:     ;
  //         PRT: }
  #pragma acc loop auto vector
  for (int i = 0; i < 2; ++i) {
    TGT_PRINTF("loop iteration\n");
    for (j = 0; j < 2; ++j)
      ;
    checkLoopSeqPrivate(*save, i, loopOnly, *loopOnly, 88, 77);
    *loopOnly = 77;
    checkLoopSeqShared(*save, i, loopOnlyArr, 88, 77);
    loopOnlyArr[0] = 77;
  }
  // Each gang will see the final j from the j loop above.
  if (j != 2)
    save->declNestedLoopFoundBadVal = true;

  //      PRT: int k = 999;
  // PRT-NEXT: int l = 888;
  int k = 999;
  int l = 888;
  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCAutoClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCVectorClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCPrivateClause {{.*}} <predetermined>
  // DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
  // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  // DMP-NEXT:     DeclRefExpr {{.*}} 'l' 'int'
  // DMP-NEXT:   impl: CompoundStmt
  // DMP-NEXT:     DeclStmt
  // DMP-NEXT:       VarDecl {{.*}} k 'int'
  // DMP-NEXT:     ForStmt
  //
  // PRT-AO-NEXT: // v----------ACC----------v
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop auto vector{{$}}
  //  PRT-A-NEXT: for (k ={{.*}}) {
  //  PRT-A-NEXT:   {{print|TGT_PRINTF}}
  //  PRT-A-NEXT:   for (l ={{.*}})
  //  PRT-A-NEXT:     ;
  //  PRT-A-NEXT: }
  // PRT-AO-NEXT: // ---------ACC->OMP--------
  // PRT-AO-NEXT: // {
  // PRT-AO-NEXT: //   int k;
  // PRT-AO-NEXT: //   for (k ={{.*}}) {
  // PRT-AO-NEXT: //     {{printf|TGT_PRINTF}}
  // PRT-AO-NEXT: //     for (l ={{.*}})
  // PRT-AO-NEXT: //       ;
  // PRT-AO-NEXT: //   }
  // PRT-AO-NEXT: // }
  // PRT-AO-NEXT: // ^----------OMP----------^
  //
  // PRT-OA-NEXT: // v----------OMP----------v
  //  PRT-O-NEXT: {
  //  PRT-O-NEXT:   int k;
  //  PRT-O-NEXT:   for (k ={{.*}}) {
  //  PRT-O-NEXT:     {{printf|TGT_PRINTF}}
  //  PRT-O-NEXT:     for (l ={{.*}})
  //  PRT-O-NEXT:       ;
  //  PRT-O-NEXT:   }
  //  PRT-O-NEXT: }
  // PRT-OA-NEXT: // ---------OMP<-ACC--------
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop auto vector{{$}}
  // PRT-OA-NEXT: // for (k ={{.*}}) {
  // PRT-OA-NEXT: //   {{printf|TGT_PRINTF}}
  // PRT-OA-NEXT: //   for (l ={{.*}})
  // PRT-OA-NEXT: //     ;
  // PRT-OA-NEXT: // }
  // PRT-OA-NEXT: // ^----------ACC----------^
  //
  // PRT-NOACC-NEXT: for (k ={{.*}}) {
  // PRT-NOACC-NEXT:   {{printf|TGT_PRINTF}}
  // PRT-NOACC-NEXT:   for (l ={{.*}})
  // PRT-NOACC-NEXT:     ;
  // PRT-NOACC-NEXT: }
  #pragma acc loop auto vector
  for (k = 1; k < 3; ++k) {
    TGT_PRINTF("loop iteration\n");
    for (l = 0; l < 2; ++l)
      ;
  }
  // Each gang will see the old k from before the loop above.
  if (k != 999)
    save->assignLoopFoundBadVal = true;
  // Each gang will see the final l from the loop above.
  if (l != 2)
    save->assignNestedLoopFoundBadVal = true;

  // PRT: k = 999;
  k = 999;
  // k should have been dropped from the loop control variable list, so it
  // should now be implicitly shared in all cases.
  //
  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCAutoClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCVectorClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  // DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
  // DMP-NEXT:   impl: ForStmt
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop auto vector
  // PRT-AO-SAME: {{^ }}// discarded in OpenMP translation
  //  PRT-A-SAME: {{^$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop auto vector // discarded in OpenMP translation{{$}}
  //    PRT-NEXT: for (int i = {{.*}}) {
  //    PRT-NEXT:   {{printf|TGT_PRINTF}}
  //    PRT-NEXT:   k = 9999;
  //    PRT-NEXT: }
  #pragma acc loop auto vector
  for (int i = 0; i < 2; ++i) {
    TGT_PRINTF("loop iteration\n");
    k = 9999;
  }
  // Each gang will see the k from the i loop above.
  if (k != 9999)
    save->writeInLaterLoopFoundBadVal = true;
}

//..............................................................................
// acc loop auto worker vector
//..............................................................................

// DMP-LABEL: FunctionDecl {{.*}} withinWorkerFnAccLoopAutoWorkerVectorRun
// PRT-LABEL: static void withinWorkerFnAccLoopAutoWorkerVectorRun({{([^()]|[[:space:]])*}})
static void withinWorkerFnAccLoopAutoWorkerVectorRun() {
  int loopOnly;
  int loopOnlyArr[1];
  struct Save save;

  //   EXE-NOT: {{.}}
  // EXE-LABEL: withinWorkerFn: acc loop auto worker vector{{$}}
  //   EXE-NOT: {{.}}
  memset(&save, 0, sizeof(save));
  loopOnly = 88;
  loopOnlyArr[0] = 88;
  printf("withinWorkerFn: acc loop auto worker vector\n");

  // Gang-redundant mode executes each loop iteration per gang.
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  #pragma acc parallel num_gangs(3)
  withinWorkerFnAccLoopAutoWorkerVector(&save, &loopOnly, loopOnlyArr);
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
}

// DMP-LABEL: FunctionDecl {{.*}} withinWorkerFnAccLoopAutoWorkerVector
// PRT-LABEL: static void withinWorkerFnAccLoopAutoWorkerVector({{([^()]|[[:space:]])*}})
static void withinWorkerFnAccLoopAutoWorkerVector(struct Save *save,
                                                  int *loopOnly,
                                                  int *loopOnlyArr) {
  // PRT: int j = 0;
  int j = 0;
  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCAutoClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCWorkerClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCVectorClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  // DMP-NEXT:     DeclRefExpr {{.*}} 'j' 'int'
  // DMP-NEXT:     DeclRefExpr {{.*}} 'save'
  // DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnly' 'int *'
  // DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnlyArr' 'int *'
  // DMP-NEXT:   impl: ForStmt
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop auto worker vector
  // PRT-AO-SAME: {{^ }}// discarded in OpenMP translation
  //  PRT-A-SAME: {{^$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop auto worker vector // discarded in OpenMP translation{{$}}
  //    PRT-NEXT: for ({{.*}}) {
  //    PRT-NEXT:   {{printf|TGT_PRINTF}}
  //    PRT-NEXT:   for (j ={{.*}})
  //    PRT-NEXT:     ;
  //         PRT: }
  #pragma acc loop auto worker vector
  for (int i = 0; i < 2; ++i) {
    TGT_PRINTF("loop iteration\n");
    for (j = 0; j < 2; ++j)
      ;
    checkLoopSeqPrivate(*save, i, loopOnly, *loopOnly, 88, 77);
    *loopOnly = 77;
    checkLoopSeqShared(*save, i, loopOnlyArr, 88, 77);
    loopOnlyArr[0] = 77;
  }
  // Each gang will see the final j from the j loop above.
  if (j != 2)
    save->declNestedLoopFoundBadVal = true;

  //      PRT: int k = 999;
  // PRT-NEXT: int l = 888;
  int k = 999;
  int l = 888;
  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCAutoClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCWorkerClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCVectorClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCPrivateClause {{.*}} <predetermined>
  // DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
  // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  // DMP-NEXT:     DeclRefExpr {{.*}} 'l' 'int'
  // DMP-NEXT:   impl: CompoundStmt
  // DMP-NEXT:     DeclStmt
  // DMP-NEXT:       VarDecl {{.*}} k 'int'
  // DMP-NEXT:     ForStmt
  //
  // PRT-AO-NEXT: // v----------ACC----------v
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop auto worker vector{{$}}
  //  PRT-A-NEXT: for (k ={{.*}}) {
  //  PRT-A-NEXT:   {{print|TGT_PRINTF}}
  //  PRT-A-NEXT:   for (l ={{.*}})
  //  PRT-A-NEXT:     ;
  //  PRT-A-NEXT: }
  // PRT-AO-NEXT: // ---------ACC->OMP--------
  // PRT-AO-NEXT: // {
  // PRT-AO-NEXT: //   int k;
  // PRT-AO-NEXT: //   for (k ={{.*}}) {
  // PRT-AO-NEXT: //     {{printf|TGT_PRINTF}}
  // PRT-AO-NEXT: //     for (l ={{.*}})
  // PRT-AO-NEXT: //       ;
  // PRT-AO-NEXT: //   }
  // PRT-AO-NEXT: // }
  // PRT-AO-NEXT: // ^----------OMP----------^
  //
  // PRT-OA-NEXT: // v----------OMP----------v
  //  PRT-O-NEXT: {
  //  PRT-O-NEXT:   int k;
  //  PRT-O-NEXT:   for (k ={{.*}}) {
  //  PRT-O-NEXT:     {{printf|TGT_PRINTF}}
  //  PRT-O-NEXT:     for (l ={{.*}})
  //  PRT-O-NEXT:       ;
  //  PRT-O-NEXT:   }
  //  PRT-O-NEXT: }
  // PRT-OA-NEXT: // ---------OMP<-ACC--------
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop auto worker vector{{$}}
  // PRT-OA-NEXT: // for (k ={{.*}}) {
  // PRT-OA-NEXT: //   {{printf|TGT_PRINTF}}
  // PRT-OA-NEXT: //   for (l ={{.*}})
  // PRT-OA-NEXT: //     ;
  // PRT-OA-NEXT: // }
  // PRT-OA-NEXT: // ^----------ACC----------^
  //
  // PRT-NOACC-NEXT: for (k ={{.*}}) {
  // PRT-NOACC-NEXT:   {{printf|TGT_PRINTF}}
  // PRT-NOACC-NEXT:   for (l ={{.*}})
  // PRT-NOACC-NEXT:     ;
  // PRT-NOACC-NEXT: }
  #pragma acc loop auto worker vector
  for (k = 1; k < 3; ++k) {
    TGT_PRINTF("loop iteration\n");
    for (l = 0; l < 2; ++l)
      ;
  }
  // Each gang will see the old k from before the loop above.
  if (k != 999)
    save->assignLoopFoundBadVal = true;
  // Each gang will see the final l from the loop above.
  if (l != 2)
    save->assignNestedLoopFoundBadVal = true;

  // PRT: k = 999;
  k = 999;
  // k should have been dropped from the loop control variable list, so it
  // should now be implicitly shared in all cases.
  //
  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCAutoClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCWorkerClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCVectorClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  // DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
  // DMP-NEXT:   impl: ForStmt
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop auto worker vector
  // PRT-AO-SAME: {{^ }}// discarded in OpenMP translation
  //  PRT-A-SAME: {{^$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop auto worker vector // discarded in OpenMP translation{{$}}
  //    PRT-NEXT: for (int i = {{.*}}) {
  //    PRT-NEXT:   {{printf|TGT_PRINTF}}
  //    PRT-NEXT:   k = 9999;
  //    PRT-NEXT: }
  #pragma acc loop auto worker vector
  for (int i = 0; i < 2; ++i) {
    TGT_PRINTF("loop iteration\n");
    k = 9999;
  }
  // Each gang will see the k from the i loop above.
  if (k != 9999)
    save->writeInLaterLoopFoundBadVal = true;
}

//..............................................................................
// acc loop INDEPENDENT
//..............................................................................

// DMP-LABEL: FunctionDecl {{.*}} withinWorkerFnAccLoopIndependentRun
// PRT-LABEL: static void withinWorkerFnAccLoopIndependentRun({{([^()]|[[:space:]])*}})
static void withinWorkerFnAccLoopIndependentRun() {
  int loopOnly;
  int loopOnlyArr[1];
  struct Save save;

  //   EXE-NOT: {{.}}
  // EXE-LABEL: withinWorkerFn: acc loop{{$}}
  //   EXE-NOT: {{.}}
  memset(&save, 0, sizeof(save));
  loopOnly = 88;
  loopOnlyArr[0] = 88;
  printf("withinWorkerFn: acc loop\n");

  // Gang-redundant mode executes each loop iteration per gang.
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  #pragma acc parallel num_gangs(3)
  withinWorkerFnAccLoopIndependent(&save, &loopOnly, loopOnlyArr);

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
}

// DMP-LABEL: FunctionDecl {{.*}} withinWorkerFnAccLoopIndependent
// PRT-LABEL: static void withinWorkerFnAccLoopIndependent({{([^()]|[[:space:]])*}})
static void withinWorkerFnAccLoopIndependent(struct Save *save, int *loopOnly,
                                             int *loopOnlyArr) {
  // PRT: int j = 0;
  int j = 0;
  //             DMP: ACCLoopDirective
  //    DMP-IND-NEXT:   ACCIndependentClause
  //     DMP-IND-NOT:     <implicit>
  // DMP-NO-IND-NEXT:   ACCIndependentClause {{.*}} <implicit>
  //        DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  //        DMP-NEXT:     DeclRefExpr {{.*}} 'j' 'int'
  //        DMP-NEXT:     DeclRefExpr {{.*}} 'save'
  //        DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnly' 'int *'
  //        DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnlyArr' 'int *'
  //        DMP-NEXT:   impl: ForStmt
  //
  //          PRT-A-NEXT: {{^ *}}#pragma acc loop
  //      PRT-A-SRC-SAME: {{^ }}INDEPENDENT
  //  PRT-A-AST-IND-SAME: {{^ }}independent
  //         PRT-AO-SAME: {{^ }}// discarded in OpenMP translation
  //          PRT-A-SAME: {{^$}}
  //         PRT-OA-NEXT: {{^ *}}// #pragma acc loop
  //     PRT-OA-SRC-SAME: {{^ }}INDEPENDENT
  // PRT-OA-AST-IND-SAME: {{^ }}independent
  //         PRT-OA-SAME: {{^ }}// discarded in OpenMP translation{{$}}
  //            PRT-NEXT: for ({{.*}}) {
  //            PRT-NEXT:   {{printf|TGT_PRINTF}}
  //            PRT-NEXT:   for (j ={{.*}})
  //            PRT-NEXT:     ;
  //                 PRT: }
  #pragma acc loop INDEPENDENT
  for (int i = 0; i < 2; ++i) {
    TGT_PRINTF("loop iteration\n");
    for (j = 0; j < 2; ++j)
      ;
    checkLoopSeqPrivate(*save, i, loopOnly, *loopOnly, 88, 77);
    *loopOnly = 77;
    checkLoopSeqShared(*save, i, loopOnlyArr, 88, 77);
    loopOnlyArr[0] = 77;
  }
  // Each gang will see the final j from the j loop above.
  if (j != 2)
    save->declNestedLoopFoundBadVal = true;

  //      PRT: int k = 999;
  // PRT-NEXT: int l = 888;
  int k = 999;
  int l = 888;
  //             DMP: ACCLoopDirective
  //    DMP-IND-NEXT:   ACCIndependentClause
  //     DMP-IND-NOT:     <implicit>
  // DMP-NO-IND-NEXT:   ACCIndependentClause {{.*}} <implicit>
  //        DMP-NEXT:   ACCPrivateClause {{.*}} <predetermined>
  //        DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
  //        DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  //        DMP-NEXT:     DeclRefExpr {{.*}} 'l' 'int'
  //        DMP-NEXT:   impl: CompoundStmt
  //        DMP-NEXT:     DeclStmt
  //        DMP-NEXT:       VarDecl {{.*}} k 'int'
  //        DMP-NEXT:     ForStmt
  //
  //        PRT-AO-NEXT: // v----------ACC----------v
  //         PRT-A-NEXT: {{^ *}}#pragma acc loop
  //     PRT-A-SRC-SAME: {{^ }}INDEPENDENT
  // PRT-A-AST-IND-SAME: {{^ }}independent
  //         PRT-A-SAME: {{^$}}
  //         PRT-A-NEXT: for (k ={{.*}}) {
  //         PRT-A-NEXT:   {{print|TGT_PRINTF}}
  //         PRT-A-NEXT:   for (l ={{.*}})
  //         PRT-A-NEXT:     ;
  //         PRT-A-NEXT: }
  //        PRT-AO-NEXT: // ---------ACC->OMP--------
  //        PRT-AO-NEXT: // {
  //        PRT-AO-NEXT: //   int k;
  //        PRT-AO-NEXT: //   for (k ={{.*}}) {
  //        PRT-AO-NEXT: //     {{printf|TGT_PRINTF}}
  //        PRT-AO-NEXT: //     for (l ={{.*}})
  //        PRT-AO-NEXT: //       ;
  //        PRT-AO-NEXT: //   }
  //        PRT-AO-NEXT: // }
  //        PRT-AO-NEXT: // ^----------OMP----------^
  //
  //         PRT-OA-NEXT: // v----------OMP----------v
  //          PRT-O-NEXT: {
  //          PRT-O-NEXT:   int k;
  //          PRT-O-NEXT:   for (k ={{.*}}) {
  //          PRT-O-NEXT:     {{printf|TGT_PRINTF}}
  //          PRT-O-NEXT:     for (l ={{.*}})
  //          PRT-O-NEXT:       ;
  //          PRT-O-NEXT:   }
  //          PRT-O-NEXT: }
  //         PRT-OA-NEXT: // ---------OMP<-ACC--------
  //         PRT-OA-NEXT: {{^ *}}// #pragma acc loop
  //     PRT-OA-SRC-SAME: {{^ }}INDEPENDENT
  // PRT-OA-AST-IND-SAME: {{^ }}independent
  //         PRT-OA-SAME: {{^$}}
  //         PRT-OA-NEXT: // for (k ={{.*}}) {
  //         PRT-OA-NEXT: //   {{printf|TGT_PRINTF}}
  //         PRT-OA-NEXT: //   for (l ={{.*}})
  //         PRT-OA-NEXT: //     ;
  //         PRT-OA-NEXT: // }
  //         PRT-OA-NEXT: // ^----------ACC----------^
  //
  // PRT-NOACC-NEXT: for (k ={{.*}}) {
  // PRT-NOACC-NEXT:   {{printf|TGT_PRINTF}}
  // PRT-NOACC-NEXT:   for (l ={{.*}})
  // PRT-NOACC-NEXT:     ;
  // PRT-NOACC-NEXT: }
  #pragma acc loop INDEPENDENT
  for (k = 1; k < 3; ++k) {
    TGT_PRINTF("loop iteration\n");
    for (l = 0; l < 2; ++l)
      ;
  }
  // Each gang will see the old k from before the loop above.
  if (k != 999)
    save->assignLoopFoundBadVal = true;
  // Each gang will see the final l from the loop above.
  if (l != 2)
    save->assignNestedLoopFoundBadVal = true;

  // PRT: k = 999;
  k = 999;
  // k should have been dropped from the loop control variable list, so it
  // should now be implicitly shared in all cases.
  //
  //             DMP: ACCLoopDirective
  //    DMP-IND-NEXT:   ACCIndependentClause
  //     DMP-IND-NOT:     <implicit>
  // DMP-NO-IND-NEXT:   ACCIndependentClause {{.*}} <implicit>
  //        DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  //        DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
  //        DMP-NEXT:   impl: ForStmt
  //
  //          PRT-A-NEXT: {{^ *}}#pragma acc loop
  //      PRT-A-SRC-SAME: {{^ }}INDEPENDENT
  //  PRT-A-AST-IND-SAME: {{^ }}independent
  //         PRT-AO-SAME: {{^ }}// discarded in OpenMP translation
  //          PRT-A-SAME: {{^$}}
  //         PRT-OA-NEXT: {{^ *}}// #pragma acc loop
  //     PRT-OA-SRC-SAME: {{^ }}INDEPENDENT
  // PRT-OA-AST-IND-SAME: {{^ }}independent
  //         PRT-OA-SAME: {{^ }}// discarded in OpenMP translation{{$}}
  //            PRT-NEXT: for (int i = {{.*}}) {
  //            PRT-NEXT:   {{printf|TGT_PRINTF}}
  //            PRT-NEXT:   k = 9999;
  //            PRT-NEXT: }
  #pragma acc loop INDEPENDENT
  for (int i = 0; i < 2; ++i) {
    TGT_PRINTF("loop iteration\n");
    k = 9999;
  }
  // Each gang will see the k from the i loop above.
  if (k != 9999)
    save->writeInLaterLoopFoundBadVal = true;
}

//------------------------------------------------------------------------------
// acc loop INDEPENDENT worker
//------------------------------------------------------------------------------

// DMP-LABEL: FunctionDecl {{.*}} withinWorkerFnAccLoopIndependentWorkerRun
// PRT-LABEL: static void withinWorkerFnAccLoopIndependentWorkerRun({{([^()]|[[:space:]])*}})
static void withinWorkerFnAccLoopIndependentWorkerRun() {
  int loopOnly;
  int loopOnlyArr[1];
  struct Save save;

  //   EXE-NOT: {{.}}
  // EXE-LABEL: withinWorkerFn: acc loop INDEPENDENT worker{{$}}
  //   EXE-NOT: {{.}}
  memset(&save, 0, sizeof(save));
  loopOnly = 88;
  loopOnlyArr[0] = 88;
  printf("withinWorkerFn: acc loop INDEPENDENT worker\n");

  // Call from a gang loop: Gang-partitioned mode executes each outer loop
  // iteration once.
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  #pragma acc parallel num_gangs(3)
  #pragma acc loop gang
  for (int g = 0; g < 2; ++g)
    withinWorkerFnAccLoopIndependentWorker(&save, &loopOnly, loopOnlyArr);
  // EXE-NEXT: save.loopOnlyFoundOldVal=1
  // EXE-NEXT: save.loopOnlyFoundBadVal=0
  // EXE-NEXT: save.loopOnlyArrFoundOldVal=1
  // EXE-NEXT: save.loopOnlyArrFoundBadVal=0
  // EXE-NEXT: save.assignLoopFoundBadVal=0
  // EXE-NEXT: save.writeInLaterLoopFoundBadVal=0
  // EXE-NEXT: loopOnly=88
  // EXE-NEXT: loopOnlyArr[0]=77
  printf("save.loopOnlyFoundOldVal=%d\n", save.loopOnlyFoundOldVal);
  printf("save.loopOnlyFoundBadVal=%d\n", save.loopOnlyFoundBadVal);
  printf("save.loopOnlyArrFoundOldVal=%d\n", save.loopOnlyArrFoundOldVal);
  printf("save.loopOnlyArrFoundBadVal=%d\n", save.loopOnlyArrFoundBadVal);
  printf("save.assignLoopFoundBadVal=%d\n", save.assignLoopFoundBadVal);
  printf("save.writeInLaterLoopFoundBadVal=%d\n", save.writeInLaterLoopFoundBadVal);
  printf("loopOnly=%d\n", loopOnly);
  printf("loopOnlyArr[0]=%d\n", loopOnlyArr[0]);

  // Call directly from the parallel construct: Gang-redundant mode executes the
  // function per gang.  Orphaned loops are the only way to achieve this
  // gang-redundant, worker-partitioned mode.
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  #pragma acc parallel num_gangs(3)
  withinWorkerFnAccLoopIndependentWorker(&save, &loopOnly, loopOnlyArr);
  // EXE-NEXT: save.loopOnlyFoundOldVal=1
  // EXE-NEXT: save.loopOnlyFoundBadVal=0
  // EXE-NEXT: save.loopOnlyArrFoundOldVal=1
  // EXE-NEXT: save.loopOnlyArrFoundBadVal=0
  // EXE-NEXT: save.assignLoopFoundBadVal=0
  // EXE-NEXT: save.writeInLaterLoopFoundBadVal=0
  // EXE-NEXT: loopOnly=88
  // EXE-NEXT: loopOnlyArr[0]=77
  printf("save.loopOnlyFoundOldVal=%d\n", save.loopOnlyFoundOldVal);
  printf("save.loopOnlyFoundBadVal=%d\n", save.loopOnlyFoundBadVal);
  printf("save.loopOnlyArrFoundOldVal=%d\n", save.loopOnlyArrFoundOldVal);
  printf("save.loopOnlyArrFoundBadVal=%d\n", save.loopOnlyArrFoundBadVal);
  printf("save.assignLoopFoundBadVal=%d\n", save.assignLoopFoundBadVal);
  printf("save.writeInLaterLoopFoundBadVal=%d\n", save.writeInLaterLoopFoundBadVal);
  printf("loopOnly=%d\n", loopOnly);
  printf("loopOnlyArr[0]=%d\n", loopOnlyArr[0]);
}

// DMP-LABEL: FunctionDecl {{.*}} withinWorkerFnAccLoopIndependentWorker
// PRT-LABEL: static void withinWorkerFnAccLoopIndependentWorker({{([^()]|[[:space:]])*}})
static void withinWorkerFnAccLoopIndependentWorker(struct Save *save,
                                                   int *loopOnly,
                                                   int *loopOnlyArr) {
  //             DMP: ACCLoopDirective
  //    DMP-IND-NEXT:   ACCIndependentClause
  //     DMP-IND-NOT:     <implicit>
  //        DMP-NEXT:   ACCWorkerClause
  //         DMP-NOT:     <implicit>
  // DMP-NO-IND-NEXT:   ACCIndependentClause {{.*}} <implicit>
  //        DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  //        DMP-NEXT:     DeclRefExpr {{.*}} 'save'
  //        DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnly' 'int *'
  //        DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnlyArr' 'int *'
  //        DMP-NEXT:   impl: OMPParallelForDirective
  //         DMP-NOT:     OMP
  //             DMP:     ForStmt
  //
  //          PRT-A-NEXT: {{^ *}}#pragma acc loop
  //      PRT-A-SRC-SAME: {{^ }}INDEPENDENT
  //  PRT-A-AST-IND-SAME: {{^ }}independent
  //          PRT-A-SAME: {{^ }}worker{{$}}
  //         PRT-AO-NEXT: {{^ *}}// #pragma omp parallel for{{$}}
  //          PRT-O-NEXT: {{^ *}}#pragma omp parallel for{{$}}
  //         PRT-OA-NEXT: {{^ *}}// #pragma acc loop
  //     PRT-OA-SRC-SAME: {{^ }}INDEPENDENT
  // PRT-OA-AST-IND-SAME: {{^ }}independent
  //         PRT-OA-SAME: {{^ }}worker{{$}}
  //            PRT-NEXT: for (int i = {{.*}}) {
  //            PRT-NEXT:   {{printf|TGT_PRINTF}}
  //                 PRT: }
  #pragma acc loop INDEPENDENT worker
  for (int i = 0; i < 2; ++i) {
    TGT_PRINTF("loop iteration\n");
    // We omit checking of an inner j loop this time.  j would be shared
    // among workers and thus would have an unpredictable value.
    checkLoopPartShared(*save, i, loopOnly, *loopOnly, 88, 77);
    *loopOnly = 77;
    checkLoopPartShared(*save, i, loopOnlyArr, loopOnlyArr[0], 88, 77);
    loopOnlyArr[0] = 77;
  }

  // PRT: int k = 999;
  int k = 999;
  //             DMP: ACCLoopDirective
  //    DMP-IND-NEXT:   ACCIndependentClause
  //     DMP-IND-NOT:     <implicit>
  //        DMP-NEXT:   ACCWorkerClause
  //         DMP-NOT:     <implicit>
  // DMP-NO-IND-NEXT:   ACCIndependentClause {{.*}} <implicit>
  //        DMP-NEXT:   ACCPrivateClause {{.*}} <predetermined>
  //        DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
  //        DMP-NEXT:   impl: OMPParallelForDirective
  //        DMP-NEXT:     OMPPrivateClause
  //         DMP-NOT:       <implicit>
  //        DMP-NEXT:       DeclRefExpr {{.*}} 'k' 'int'
  //             DMP:     ForStmt
  //
  //          PRT-A-NEXT: {{^ *}}#pragma acc loop
  //      PRT-A-SRC-SAME: {{^ }}INDEPENDENT
  //  PRT-A-AST-IND-SAME: {{^ }}independent
  //          PRT-A-SAME: {{^ }}worker{{$}}
  //         PRT-AO-NEXT: {{^ *}}// #pragma omp parallel for private(k){{$}}
  //          PRT-O-NEXT: {{^ *}}#pragma omp parallel for private(k){{$}}
  //         PRT-OA-NEXT: {{^ *}}// #pragma acc loop
  //     PRT-OA-SRC-SAME: {{^ }}INDEPENDENT
  // PRT-OA-AST-IND-SAME: {{^ }}independent
  //         PRT-OA-SAME: {{^ }}worker{{$}}
  //            PRT-NEXT: for (k ={{.*}}) {
  //            PRT-NEXT:   {{printf|TGT_PRINTF}}
  //            PRT-NEXT: }
  #pragma acc loop INDEPENDENT worker
  for (k = 1; k < 3; ++k) {
    TGT_PRINTF("loop iteration\n");
    // We omit checking of an inner l loop this time.  l would be shared
    // among workers and thus would have an unpredictable value.
  }
  // Each gang will see the old k from before the loop above.
  if (k != 999)
    save->assignLoopFoundBadVal = true;

  // PRT: k = 999;
  k = 999;
  // k should have been dropped from the loop control variable list, so it
  // should now be implicitly shared in all cases.
  //
  //             DMP: ACCLoopDirective
  //    DMP-IND-NEXT:   ACCIndependentClause
  //     DMP-IND-NOT:     <implicit>
  //        DMP-NEXT:   ACCWorkerClause
  //         DMP-NOT:     <implicit>
  // DMP-NO-IND-NEXT:   ACCIndependentClause {{.*}} <implicit>
  //        DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  //        DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
  //        DMP-NEXT:   impl: OMPParallelForDirective
  //         DMP-NOT:     OMP
  //             DMP:     ForStmt
  //
  //          PRT-A-NEXT: {{^ *}}#pragma acc loop
  //      PRT-A-SRC-SAME: {{^ }}INDEPENDENT
  //  PRT-A-AST-IND-SAME: {{^ }}independent
  //          PRT-A-SAME: {{^ }}worker{{$}}
  //         PRT-AO-NEXT: {{^ *}}// #pragma omp parallel for{{$}}
  //          PRT-O-NEXT: {{^ *}}#pragma omp parallel for{{$}}
  //         PRT-OA-NEXT: {{^ *}}// #pragma acc loop
  //     PRT-OA-SRC-SAME: {{^ }}INDEPENDENT
  // PRT-OA-AST-IND-SAME: {{^ }}independent
  //         PRT-OA-SAME: {{^ }}worker{{$}}
  //            PRT-NEXT: for ({{.*}}) {
  //            PRT-NEXT:   {{printf|TGT_PRINTF}}
  //            PRT-NEXT:   k = 9999;
  //            PRT-NEXT: }
  #pragma acc loop INDEPENDENT worker
  for (int i = 0; i < 2; ++i) {
    TGT_PRINTF("loop iteration\n");
    k = 9999;
  }
  // Each gang that calls this function will see the k from the i loop above.
  if (k != 9999)
    save->writeInLaterLoopFoundBadVal = true;
}

//------------------------------------------------------------------------------
// acc loop INDEPENDENT vector
//------------------------------------------------------------------------------

// DMP-LABEL: FunctionDecl {{.*}} withinWorkerFnAccLoopIndependentVectorRun
// PRT-LABEL: static void withinWorkerFnAccLoopIndependentVectorRun({{([^()]|[[:space:]])*}})
static void withinWorkerFnAccLoopIndependentVectorRun() {
  int loopOnly;
  int loopOnlyArr[1];
  struct Save save;

  //   EXE-NOT: {{.}}
  // EXE-LABEL: withinWorkerFn: acc loop INDEPENDENT vector{{$}}
  //   EXE-NOT: {{.}}
  memset(&save, 0, sizeof(save));
  loopOnly = 88;
  loopOnlyArr[0] = 88;
  printf("withinWorkerFn: acc loop INDEPENDENT vector\n");

  // Call from a gang loop: Gang-partitioned mode executes each outer loop
  // iteration once.
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  #pragma acc parallel num_gangs(3)
  #pragma acc loop gang
  for (int g = 0; g < 2; ++g)
    withinWorkerFnAccLoopIndependentVector(&save, &loopOnly, loopOnlyArr);
  // EXE-NEXT: save.loopOnlyFoundOldVal=1
  // EXE-NEXT: save.loopOnlyFoundBadVal=0
  // EXE-NEXT: save.loopOnlyArrFoundOldVal=1
  // EXE-NEXT: save.loopOnlyArrFoundBadVal=0
  // EXE-NEXT: save.assignLoopFoundBadVal=0
  // EXE-NEXT: save.writeInLaterLoopFoundBadVal=0
  // EXE-NEXT: loopOnly=88
  // EXE-NEXT: loopOnlyArr[0]=77
  printf("save.loopOnlyFoundOldVal=%d\n", save.loopOnlyFoundOldVal);
  printf("save.loopOnlyFoundBadVal=%d\n", save.loopOnlyFoundBadVal);
  printf("save.loopOnlyArrFoundOldVal=%d\n", save.loopOnlyArrFoundOldVal);
  printf("save.loopOnlyArrFoundBadVal=%d\n", save.loopOnlyArrFoundBadVal);
  printf("save.assignLoopFoundBadVal=%d\n", save.assignLoopFoundBadVal);
  printf("save.writeInLaterLoopFoundBadVal=%d\n", save.writeInLaterLoopFoundBadVal);
  printf("loopOnly=%d\n", loopOnly);
  printf("loopOnlyArr[0]=%d\n", loopOnlyArr[0]);

  // Call directly from the parallel construct: Gang-redundant mode executes the
  // function per gang.  Orphaned loops are the only way to achieve this
  // gang-redundant, vector-partitioned mode.
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  #pragma acc parallel num_gangs(3)
  withinWorkerFnAccLoopIndependentVector(&save, &loopOnly, loopOnlyArr);
  // EXE-NEXT: save.loopOnlyFoundOldVal=1
  // EXE-NEXT: save.loopOnlyFoundBadVal=0
  // EXE-NEXT: save.loopOnlyArrFoundOldVal=1
  // EXE-NEXT: save.loopOnlyArrFoundBadVal=0
  // EXE-NEXT: save.assignLoopFoundBadVal=0
  // EXE-NEXT: save.writeInLaterLoopFoundBadVal=0
  // EXE-NEXT: loopOnly=88
  // EXE-NEXT: loopOnlyArr[0]=77
  printf("save.loopOnlyFoundOldVal=%d\n", save.loopOnlyFoundOldVal);
  printf("save.loopOnlyFoundBadVal=%d\n", save.loopOnlyFoundBadVal);
  printf("save.loopOnlyArrFoundOldVal=%d\n", save.loopOnlyArrFoundOldVal);
  printf("save.loopOnlyArrFoundBadVal=%d\n", save.loopOnlyArrFoundBadVal);
  printf("save.assignLoopFoundBadVal=%d\n", save.assignLoopFoundBadVal);
  printf("save.writeInLaterLoopFoundBadVal=%d\n", save.writeInLaterLoopFoundBadVal);
  printf("loopOnly=%d\n", loopOnly);
  printf("loopOnlyArr[0]=%d\n", loopOnlyArr[0]);
}

// DMP-LABEL: FunctionDecl {{.*}} withinWorkerFnAccLoopIndependentVector
// PRT-LABEL: static void withinWorkerFnAccLoopIndependentVector({{([^()]|[[:space:]])*}})
static void withinWorkerFnAccLoopIndependentVector(struct Save *save,
                                                   int *loopOnly,
                                                   int *loopOnlyArr) {
  //             DMP: ACCLoopDirective
  //    DMP-IND-NEXT:   ACCIndependentClause
  //     DMP-IND-NOT:     <implicit>
  //        DMP-NEXT:   ACCVectorClause
  //         DMP-NOT:     <implicit>
  // DMP-NO-IND-NEXT:   ACCIndependentClause {{.*}} <implicit>
  //        DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  //        DMP-NEXT:     DeclRefExpr {{.*}} 'save'
  //        DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnly' 'int *'
  //        DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnlyArr' 'int *'
  //        DMP-NEXT:   impl: OMPSimdDirective
  //         DMP-NOT:     OMP
  //             DMP:     ForStmt
  //
  //          PRT-A-NEXT: {{^ *}}#pragma acc loop
  //      PRT-A-SRC-SAME: {{^ }}INDEPENDENT
  //  PRT-A-AST-IND-SAME: {{^ }}independent
  //          PRT-A-SAME: {{^ }}vector{{$}}
  //         PRT-AO-NEXT: {{^ *}}// #pragma omp simd{{$}}
  //          PRT-O-NEXT: {{^ *}}#pragma omp simd{{$}}
  //         PRT-OA-NEXT: {{^ *}}// #pragma acc loop
  //     PRT-OA-SRC-SAME: {{^ }}INDEPENDENT
  // PRT-OA-AST-IND-SAME: {{^ }}independent
  //         PRT-OA-SAME: {{^ }}vector{{$}}
  //            PRT-NEXT: for (int i = {{.*}}) {
  //            PRT-NEXT:   {{printf|TGT_PRINTF}}
  //                 PRT: }
  #pragma acc loop INDEPENDENT vector
  for (int i = 0; i < 2; ++i) {
    TGT_PRINTF("loop iteration\n");
    // We omit checking of an inner j loop this time.  j would be shared
    // among vector lanes and thus would have an unpredictable value.
    checkLoopPartShared(*save, i, loopOnly, *loopOnly, 88, 77);
    *loopOnly = 77;
    checkLoopPartShared(*save, i, loopOnlyArr, loopOnlyArr[0], 88, 77);
    loopOnlyArr[0] = 77;
  }

  // PRT: int k = 999;
  int k = 999;
  //              DMP: ACCLoopDirective
  //     DMP-IND-NEXT:   ACCIndependentClause
  //      DMP-IND-NOT:     <implicit>
  //         DMP-NEXT:   ACCVectorClause
  //          DMP-NOT:     <implicit>
  //  DMP-NO-IND-NEXT:   ACCIndependentClause {{.*}} <implicit>
  //         DMP-NEXT:   ACCPrivateClause {{.*}} <predetermined>
  //         DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
  //         DMP-NEXT:   impl: CompoundStmt
  //         DMP-NEXT:     DeclStmt
  //         DMP-NEXT:       VarDecl {{.*}} k 'int'
  //         DMP-NEXT:     OMPSimdDirective
  //          DMP-NOT:       OMPPrivateClause
  //              DMP:       ForStmt
  //
  //         PRT-AO-NEXT: // v----------ACC----------v
  //          PRT-A-NEXT: {{^ *}}#pragma acc loop
  //      PRT-A-SRC-SAME: {{^ }}INDEPENDENT
  //  PRT-A-AST-IND-SAME: {{^ }}independent
  //          PRT-A-SAME: {{^ }}vector{{$}}
  //          PRT-A-NEXT: for (k ={{.*}}) {
  //          PRT-A-NEXT:   {{print|TGT_PRINTF}}
  //          PRT-A-NEXT: }
  //         PRT-AO-NEXT: // ---------ACC->OMP--------
  //         PRT-AO-NEXT: // {
  //         PRT-AO-NEXT: //   int k;
  //         PRT-AO-NEXT: //   #pragma omp simd{{$}}
  //         PRT-AO-NEXT: //   for (k ={{.*}}) {
  //         PRT-AO-NEXT: //     {{printf|TGT_PRINTF}}
  //         PRT-AO-NEXT: //   }
  //         PRT-AO-NEXT: // }
  //         PRT-AO-NEXT: // ^----------OMP----------^
  //
  //         PRT-OA-NEXT: // v----------OMP----------v
  //          PRT-O-NEXT: {
  //          PRT-O-NEXT:   int k;
  //          PRT-O-NEXT:   {{^ *}}#pragma omp simd{{$}}
  //          PRT-O-NEXT:   for (k ={{.*}}) {
  //          PRT-O-NEXT:     {{printf|TGT_PRINTF}}
  //          PRT-O-NEXT:   }
  //          PRT-O-NEXT: }
  //         PRT-OA-NEXT: // ---------OMP<-ACC--------
  //         PRT-OA-NEXT: {{^ *}}// #pragma acc loop
  //     PRT-OA-SRC-SAME: {{^ }}INDEPENDENT
  // PRT-OA-AST-IND-SAME: {{^ }}independent
  //         PRT-OA-SAME: {{^ }}vector{{$}}
  //         PRT-OA-NEXT: // for (k ={{.*}}) {
  //         PRT-OA-NEXT: //   {{printf|TGT_PRINTF}}
  //         PRT-OA-NEXT: // }
  //         PRT-OA-NEXT: // ^----------ACC----------^
  //
  // PRT-NOACC-NEXT: for (k ={{.*}}) {
  // PRT-NOACC-NEXT:   {{printf|TGT_PRINTF}}
  // PRT-NOACC-NEXT: }
  #pragma acc loop INDEPENDENT vector
  for (k = 1; k < 3; ++k) {
    TGT_PRINTF("loop iteration\n");
    // We omit checking of an inner l loop this time.  l would be shared
    // among vector lanes and thus would have an unpredictable value.
  }
  // Each gang will see the old k from before the loop above.
  if (k != 999)
    save->assignLoopFoundBadVal = true;

  // PRT: k = 999;
  k = 999;
  // k should have been dropped from the loop control variable list, so it
  // should now be implicitly shared in all cases.
  //
  //             DMP: ACCLoopDirective
  //    DMP-IND-NEXT:   ACCIndependentClause
  //     DMP-IND-NOT:     <implicit>
  //        DMP-NEXT:   ACCVectorClause
  //         DMP-NOT:     <implicit>
  // DMP-NO-IND-NEXT:   ACCIndependentClause {{.*}} <implicit>
  //        DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  //        DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
  //        DMP-NEXT:   impl: OMPSimdDirective
  //         DMP-NOT:     OMP
  //             DMP:     ForStmt
  //
  //          PRT-A-NEXT: {{^ *}}#pragma acc loop
  //      PRT-A-SRC-SAME: {{^ }}INDEPENDENT
  //  PRT-A-AST-IND-SAME: {{^ }}independent
  //          PRT-A-SAME: {{^ }}vector{{$}}
  //         PRT-AO-NEXT: {{^ *}}// #pragma omp simd{{$}}
  //          PRT-O-NEXT: {{^ *}}#pragma omp simd{{$}}
  //         PRT-OA-NEXT: {{^ *}}// #pragma acc loop
  //     PRT-OA-SRC-SAME: {{^ }}INDEPENDENT
  // PRT-OA-AST-IND-SAME: {{^ }}independent
  //         PRT-OA-SAME: {{^ }}vector{{$}}
  //            PRT-NEXT: for ({{.*}}) {
  //            PRT-NEXT:   {{printf|TGT_PRINTF}}
  //            PRT-NEXT:   k = 9999;
  //            PRT-NEXT: }
  #pragma acc loop INDEPENDENT vector
  for (int i = 0; i < 2; ++i) {
    TGT_PRINTF("loop iteration\n");
    k = 9999;
  }
  // Each gang that calls this function will see the k from the i loop above.
  if (k != 9999)
    save->writeInLaterLoopFoundBadVal = true;
}

//------------------------------------------------------------------------------
// acc loop INDEPENDENT worker vector
//------------------------------------------------------------------------------

// DMP-LABEL: FunctionDecl {{.*}} withinWorkerFnAccLoopIndependentWorkerVectorRun
// PRT-LABEL: static void withinWorkerFnAccLoopIndependentWorkerVectorRun({{([^()]|[[:space:]])*}})
static void withinWorkerFnAccLoopIndependentWorkerVectorRun() {
  int loopOnly;
  int loopOnlyArr[1];
  struct Save save;

  //   EXE-NOT: {{.}}
  // EXE-LABEL: withinWorkerFn: acc loop INDEPENDENT worker vector{{$}}
  //   EXE-NOT: {{.}}
  memset(&save, 0, sizeof(save));
  loopOnly = 88;
  loopOnlyArr[0] = 88;
  printf("withinWorkerFn: acc loop INDEPENDENT worker vector\n");

  // Call from a gang loop: Gang-partitioned mode executes each outer loop
  // iteration once.
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  #pragma acc parallel num_gangs(3)
  #pragma acc loop gang
  for (int g = 0; g < 2; ++g)
    withinWorkerFnAccLoopIndependentWorkerVector(&save, &loopOnly, loopOnlyArr);
  // EXE-NEXT: save.loopOnlyFoundOldVal=1
  // EXE-NEXT: save.loopOnlyFoundBadVal=0
  // EXE-NEXT: save.loopOnlyArrFoundOldVal=1
  // EXE-NEXT: save.loopOnlyArrFoundBadVal=0
  // EXE-NEXT: save.assignLoopFoundBadVal=0
  // EXE-NEXT: save.writeInLaterLoopFoundBadVal=0
  // EXE-NEXT: loopOnly=88
  // EXE-NEXT: loopOnlyArr[0]=77
  printf("save.loopOnlyFoundOldVal=%d\n", save.loopOnlyFoundOldVal);
  printf("save.loopOnlyFoundBadVal=%d\n", save.loopOnlyFoundBadVal);
  printf("save.loopOnlyArrFoundOldVal=%d\n", save.loopOnlyArrFoundOldVal);
  printf("save.loopOnlyArrFoundBadVal=%d\n", save.loopOnlyArrFoundBadVal);
  printf("save.assignLoopFoundBadVal=%d\n", save.assignLoopFoundBadVal);
  printf("save.writeInLaterLoopFoundBadVal=%d\n", save.writeInLaterLoopFoundBadVal);
  printf("loopOnly=%d\n", loopOnly);
  printf("loopOnlyArr[0]=%d\n", loopOnlyArr[0]);

  // Call directly from the parallel construct: Gang-redundant mode executes the
  // function per gang.  Orphaned loops are the only way to achieve this
  // gang-redundant, worker-partitioned, vector-partitioned mode.
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  #pragma acc parallel num_gangs(3)
  withinWorkerFnAccLoopIndependentWorkerVector(&save, &loopOnly, loopOnlyArr);
  // EXE-NEXT: save.loopOnlyFoundOldVal=1
  // EXE-NEXT: save.loopOnlyFoundBadVal=0
  // EXE-NEXT: save.loopOnlyArrFoundOldVal=1
  // EXE-NEXT: save.loopOnlyArrFoundBadVal=0
  // EXE-NEXT: save.assignLoopFoundBadVal=0
  // EXE-NEXT: save.writeInLaterLoopFoundBadVal=0
  // EXE-NEXT: loopOnly=88
  // EXE-NEXT: loopOnlyArr[0]=77
  printf("save.loopOnlyFoundOldVal=%d\n", save.loopOnlyFoundOldVal);
  printf("save.loopOnlyFoundBadVal=%d\n", save.loopOnlyFoundBadVal);
  printf("save.loopOnlyArrFoundOldVal=%d\n", save.loopOnlyArrFoundOldVal);
  printf("save.loopOnlyArrFoundBadVal=%d\n", save.loopOnlyArrFoundBadVal);
  printf("save.assignLoopFoundBadVal=%d\n", save.assignLoopFoundBadVal);
  printf("save.writeInLaterLoopFoundBadVal=%d\n", save.writeInLaterLoopFoundBadVal);
  printf("loopOnly=%d\n", loopOnly);
  printf("loopOnlyArr[0]=%d\n", loopOnlyArr[0]);
}

// DMP-LABEL: FunctionDecl {{.*}} withinWorkerFnAccLoopIndependentWorkerVector
// PRT-LABEL: static void withinWorkerFnAccLoopIndependentWorkerVector({{([^()]|[[:space:]])*}})
static void withinWorkerFnAccLoopIndependentWorkerVector(struct Save *save,
                                                         int *loopOnly,
                                                         int *loopOnlyArr) {
  //             DMP: ACCLoopDirective
  //    DMP-IND-NEXT:   ACCIndependentClause
  //     DMP-IND-NOT:     <implicit>
  //        DMP-NEXT:   ACCWorkerClause
  //         DMP-NOT:     <implicit>
  //        DMP-NEXT:   ACCVectorClause
  //         DMP-NOT:     <implicit>
  // DMP-NO-IND-NEXT:   ACCIndependentClause {{.*}} <implicit>
  //        DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  //        DMP-NEXT:     DeclRefExpr {{.*}} 'save'
  //        DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnly' 'int *'
  //        DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnlyArr' 'int *'
  //        DMP-NEXT:   impl: OMPParallelForSimdDirective
  //         DMP-NOT:     OMP
  //             DMP:     ForStmt
  //
  //          PRT-A-NEXT: {{^ *}}#pragma acc loop
  //      PRT-A-SRC-SAME: {{^ }}INDEPENDENT
  //  PRT-A-AST-IND-SAME: {{^ }}independent
  //          PRT-A-SAME: {{^ }}worker vector{{$}}
  //         PRT-AO-NEXT: {{^ *}}// #pragma omp parallel for simd{{$}}
  //          PRT-O-NEXT: {{^ *}}#pragma omp parallel for simd{{$}}
  //         PRT-OA-NEXT: {{^ *}}// #pragma acc loop
  //     PRT-OA-SRC-SAME: {{^ }}INDEPENDENT
  // PRT-OA-AST-IND-SAME: {{^ }}independent
  //         PRT-OA-SAME: {{^ }}worker vector{{$}}
  //            PRT-NEXT: for (int i = {{.*}}) {
  //            PRT-NEXT:   {{printf|TGT_PRINTF}}
  //                 PRT: }
  #pragma acc loop INDEPENDENT worker vector
  for (int i = 0; i < 2; ++i) {
    TGT_PRINTF("loop iteration\n");
    // We omit checking of an inner j loop this time.  j would be shared
    // among workers and vector lanes and thus would have an unpredictable
    // value.
    checkLoopPartShared(*save, i, loopOnly, *loopOnly, 88, 77);
    *loopOnly = 77;
    checkLoopPartShared(*save, i, loopOnlyArr, loopOnlyArr[0], 88, 77);
    loopOnlyArr[0] = 77;
  }

  // PRT: int k = 999;
  int k = 999;
  //             DMP: ACCLoopDirective
  //    DMP-IND-NEXT:   ACCIndependentClause
  //     DMP-IND-NOT:     <implicit>
  //        DMP-NEXT:   ACCWorkerClause
  //         DMP-NOT:     <implicit>
  //        DMP-NEXT:   ACCVectorClause
  //         DMP-NOT:     <implicit>
  // DMP-NO-IND-NEXT:   ACCIndependentClause {{.*}} <implicit>
  //        DMP-NEXT:   ACCPrivateClause {{.*}} <predetermined>
  //        DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
  //        DMP-NEXT:   impl: CompoundStmt
  //        DMP-NEXT:     DeclStmt
  //        DMP-NEXT:       VarDecl {{.*}} k 'int'
  //        DMP-NEXT:     OMPParallelForSimdDirective
  //         DMP-NOT:       OMPPrivateClause
  //             DMP:       ForStmt
  //
  //         PRT-AO-NEXT: // v----------ACC----------v
  //          PRT-A-NEXT: {{^ *}}#pragma acc loop
  //      PRT-A-SRC-SAME: {{^ }}INDEPENDENT
  //  PRT-A-AST-IND-SAME: {{^ }}independent
  //          PRT-A-SAME: {{^ }}worker vector{{$}}
  //          PRT-A-NEXT: for (k ={{.*}}) {
  //          PRT-A-NEXT:   {{print|TGT_PRINTF}}
  //          PRT-A-NEXT: }
  //         PRT-AO-NEXT: // ---------ACC->OMP--------
  //         PRT-AO-NEXT: // {
  //         PRT-AO-NEXT: //   int k;
  //         PRT-AO-NEXT: //   #pragma omp parallel for simd{{$}}
  //         PRT-AO-NEXT: //   for (k ={{.*}}) {
  //         PRT-AO-NEXT: //     {{printf|TGT_PRINTF}}
  //         PRT-AO-NEXT: //   }
  //         PRT-AO-NEXT: // }
  //         PRT-AO-NEXT: // ^----------OMP----------^
  //
  //         PRT-OA-NEXT: // v----------OMP----------v
  //          PRT-O-NEXT: {
  //          PRT-O-NEXT:   int k;
  //          PRT-O-NEXT:   {{^ *}}#pragma omp parallel for simd{{$}}
  //          PRT-O-NEXT:   for (k ={{.*}}) {
  //          PRT-O-NEXT:     {{printf|TGT_PRINTF}}
  //          PRT-O-NEXT:   }
  //          PRT-O-NEXT: }
  //         PRT-OA-NEXT: // ---------OMP<-ACC--------
  //         PRT-OA-NEXT: {{^ *}}// #pragma acc loop
  //     PRT-OA-SRC-SAME: {{^ }}INDEPENDENT
  // PRT-OA-AST-IND-SAME: {{^ }}independent
  //         PRT-OA-SAME: {{^ }}worker vector{{$}}
  //         PRT-OA-NEXT: // for (k ={{.*}}) {
  //         PRT-OA-NEXT: //   {{printf|TGT_PRINTF}}
  //         PRT-OA-NEXT: // }
  //         PRT-OA-NEXT: // ^----------ACC----------^
  //
  // PRT-NOACC-NEXT: for (k ={{.*}}) {
  // PRT-NOACC-NEXT:   {{printf|TGT_PRINTF}}
  // PRT-NOACC-NEXT: }
  #pragma acc loop INDEPENDENT worker vector
  for (k = 1; k < 3; ++k) {
    TGT_PRINTF("loop iteration\n");
    // We omit checking of an inner l loop this time.  l would be shared
    // among workers and vector lanes and thus would have an unpredictable
    // value.
  }
  // Each gang will see the old k from before the loop above.
  if (k != 999)
    save->assignLoopFoundBadVal = true;

  // PRT: k = 999;
  k = 999;
  // k should have been dropped from the loop control variable list, so it
  // should now be implicitly shared in all cases.
  //
  //              DMP: ACCLoopDirective
  //     DMP-IND-NEXT:   ACCIndependentClause
  //      DMP-IND-NOT:     <implicit>
  //         DMP-NEXT:   ACCWorkerClause
  //          DMP-NOT:     <implicit>
  //         DMP-NEXT:   ACCVectorClause
  //          DMP-NOT:     <implicit>
  //  DMP-NO-IND-NEXT:   ACCIndependentClause {{.*}} <implicit>
  //         DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  //         DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
  //         DMP-NEXT:   impl: OMPParallelForSimdDirective
  //          DMP-NOT:     OMP
  //              DMP:     ForStmt
  //
  //           PRT-A-NEXT: {{^ *}}#pragma acc loop
  //       PRT-A-SRC-SAME: {{^ }}INDEPENDENT
  //   PRT-A-AST-IND-SAME: {{^ }}independent
  //           PRT-A-SAME: {{^ }}worker vector{{$}}
  //          PRT-AO-NEXT: {{^ *}}// #pragma omp parallel for simd{{$}}
  //           PRT-O-NEXT: {{^ *}}#pragma omp parallel for simd{{$}}
  //          PRT-OA-NEXT: {{^ *}}// #pragma acc loop
  //      PRT-OA-SRC-SAME: {{^ }}INDEPENDENT
  //  PRT-OA-AST-IND-SAME: {{^ }}independent
  //          PRT-OA-SAME: {{^ }}worker vector{{$}}
  //             PRT-NEXT: for ({{.*}}) {
  //             PRT-NEXT:   {{printf|TGT_PRINTF}}
  //             PRT-NEXT:   k = 9999;
  //             PRT-NEXT: }
  #pragma acc loop INDEPENDENT worker vector
  for (int i = 0; i < 2; ++i) {
    TGT_PRINTF("loop iteration\n");
    k = 9999;
  }
  // Each gang that calls this function will see the k from the i loop above.
  if (k != 9999)
    save->writeInLaterLoopFoundBadVal = true;
}

//==============================================================================
// Check within vector functions.
//
// This repeats the above testing but within a vector function.
//
// In all cases, check calling vector functions directly from the enclosing
// parallel construct.  For sequential loops within vector functions, that
// should be sufficient to prove the loops generally execute sequentially.  For
// partitioned loops within vector functions, also check calling the vector
// functions from gang loops and worker loops to make sure nesting parallelism
// levels in that way works correctly at run time.
//==============================================================================

//------------------------------------------------------------------------------
// Sequential loops.
//------------------------------------------------------------------------------

//..............................................................................
// acc loop seq
//..............................................................................

// DMP-LABEL: FunctionDecl {{.*}} withinVectorFnAccLoopSeqRun
// PRT-LABEL: static void withinVectorFnAccLoopSeqRun({{([^()]|[[:space:]])*}})
static void withinVectorFnAccLoopSeqRun() {
  int loopOnly;
  int loopOnlyArr[1];
  struct Save save;

  //   EXE-NOT: {{.}}
  // EXE-LABEL: withinVectorFn: acc loop seq{{$}}
  //   EXE-NOT: {{.}}
  memset(&save, 0, sizeof(save));
  loopOnly = 88;
  loopOnlyArr[0] = 88;
  printf("withinVectorFn: acc loop seq\n");

  // Gang-redundant mode executes each loop iteration per gang.
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  #pragma acc parallel num_gangs(3)
  withinVectorFnAccLoopSeq(&save, &loopOnly, loopOnlyArr);
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
}

// DMP-LABEL: FunctionDecl {{.*}} withinVectorFnAccLoopSeq
// PRT-LABEL: static void withinVectorFnAccLoopSeq({{([^()]|[[:space:]])*}})
static void withinVectorFnAccLoopSeq(struct Save *save, int *loopOnly,
                                     int *loopOnlyArr) {
  // PRT: int j = 0;
  int j = 0;
  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCSeqClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  // DMP-NEXT:     DeclRefExpr {{.*}} 'j' 'int'
  // DMP-NEXT:     DeclRefExpr {{.*}} 'save'
  // DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnly' 'int *'
  // DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnlyArr' 'int *'
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
  #pragma acc loop seq
  for (int i = 0; i < 2; ++i) {
    TGT_PRINTF("loop iteration\n");
    for (j = 0; j < 2; ++j)
      ;
    checkLoopSeqPrivate(*save, i, loopOnly, *loopOnly, 88, 77);
    *loopOnly = 77;
    checkLoopSeqShared(*save, i, loopOnlyArr, 88, 77);
    loopOnlyArr[0] = 77;
  }
  // Each gang will see the final j from the j loop above.
  if (j != 2)
    save->declNestedLoopFoundBadVal = true;

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
  #pragma acc loop seq
  for (k = 1; k < 3; ++k) {
    TGT_PRINTF("loop iteration\n");
    for (l = 0; l < 2; ++l)
      ;
  }
  // Each gang will see the final k and l from the loop above.
  if (k != 3)
    save->assignLoopFoundBadVal = true;
  if (l != 2)
    save->assignNestedLoopFoundBadVal = true;
}

//..............................................................................
// acc loop auto
//..............................................................................

// DMP-LABEL: FunctionDecl {{.*}} withinVectorFnAccLoopAutoRun
// PRT-LABEL: static void withinVectorFnAccLoopAutoRun({{([^()]|[[:space:]])*}})
static void withinVectorFnAccLoopAutoRun() {
  int loopOnly;
  int loopOnlyArr[1];
  struct Save save;

  //   EXE-NOT: {{.}}
  // EXE-LABEL: withinVectorFn: acc loop auto{{$}}
  //   EXE-NOT: {{.}}
  memset(&save, 0, sizeof(save));
  loopOnly = 88;
  loopOnlyArr[0] = 88;
  printf("withinVectorFn: acc loop auto\n");

  // Gang-redundant mode executes each loop iteration per gang.
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  #pragma acc parallel num_gangs(3)
  withinVectorFnAccLoopAuto(&save, &loopOnly, loopOnlyArr);

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
}

// DMP-LABEL: FunctionDecl {{.*}} withinVectorFnAccLoopAuto
// PRT-LABEL: static void withinVectorFnAccLoopAuto({{([^()]|[[:space:]])*}})
static void withinVectorFnAccLoopAuto(struct Save *save, int *loopOnly,
                                      int *loopOnlyArr) {
  // PRT: int j = 0;
  int j = 0;
  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCAutoClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  // DMP-NEXT:     DeclRefExpr {{.*}} 'j' 'int'
  // DMP-NEXT:     DeclRefExpr {{.*}} 'save'
  // DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnly' 'int *'
  // DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnlyArr' 'int *'
  // DMP-NEXT:   impl: ForStmt
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop auto
  // PRT-AO-SAME: {{^ }}// discarded in OpenMP translation
  //  PRT-A-SAME: {{^$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop auto // discarded in OpenMP translation{{$}}
  //    PRT-NEXT: for ({{.*}}) {
  //    PRT-NEXT:   {{printf|TGT_PRINTF}}
  //    PRT-NEXT:   for (j ={{.*}})
  //    PRT-NEXT:     ;
  //         PRT: }
  #pragma acc loop auto
  for (int i = 0; i < 2; ++i) {
    TGT_PRINTF("loop iteration\n");
    for (j = 0; j < 2; ++j)
      ;
    checkLoopSeqPrivate(*save, i, loopOnly, *loopOnly, 88, 77);
    *loopOnly = 77;
    checkLoopSeqShared(*save, i, loopOnlyArr, 88, 77);
    loopOnlyArr[0] = 77;
  }
  // Each gang will see the final j from the j loop above.
  if (j != 2)
    save->declNestedLoopFoundBadVal = true;

  //      PRT: int k = 999;
  // PRT-NEXT: int l = 888;
  int k = 999;
  int l = 888;
  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCAutoClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCPrivateClause {{.*}} <predetermined>
  // DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
  // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  // DMP-NEXT:     DeclRefExpr {{.*}} 'l' 'int'
  // DMP-NEXT:   impl: CompoundStmt
  // DMP-NEXT:     DeclStmt
  // DMP-NEXT:       VarDecl {{.*}} k 'int'
  // DMP-NEXT:     ForStmt
  //
  // PRT-AO-NEXT: // v----------ACC----------v
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop auto{{$}}
  //  PRT-A-NEXT: for (k ={{.*}}) {
  //  PRT-A-NEXT:   {{print|TGT_PRINTF}}
  //  PRT-A-NEXT:   for (l ={{.*}})
  //  PRT-A-NEXT:     ;
  //  PRT-A-NEXT: }
  // PRT-AO-NEXT: // ---------ACC->OMP--------
  // PRT-AO-NEXT: // {
  // PRT-AO-NEXT: //   int k;
  // PRT-AO-NEXT: //   for (k ={{.*}}) {
  // PRT-AO-NEXT: //     {{printf|TGT_PRINTF}}
  // PRT-AO-NEXT: //     for (l ={{.*}})
  // PRT-AO-NEXT: //       ;
  // PRT-AO-NEXT: //   }
  // PRT-AO-NEXT: // }
  // PRT-AO-NEXT: // ^----------OMP----------^
  //
  // PRT-OA-NEXT: // v----------OMP----------v
  //  PRT-O-NEXT: {
  //  PRT-O-NEXT:   int k;
  //  PRT-O-NEXT:   for (k ={{.*}}) {
  //  PRT-O-NEXT:     {{printf|TGT_PRINTF}}
  //  PRT-O-NEXT:     for (l ={{.*}})
  //  PRT-O-NEXT:       ;
  //  PRT-O-NEXT:   }
  //  PRT-O-NEXT: }
  // PRT-OA-NEXT: // ---------OMP<-ACC--------
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop auto{{$}}
  // PRT-OA-NEXT: // for (k ={{.*}}) {
  // PRT-OA-NEXT: //   {{printf|TGT_PRINTF}}
  // PRT-OA-NEXT: //   for (l ={{.*}})
  // PRT-OA-NEXT: //     ;
  // PRT-OA-NEXT: // }
  // PRT-OA-NEXT: // ^----------ACC----------^
  //
  // PRT-NOACC-NEXT: for (k ={{.*}}) {
  // PRT-NOACC-NEXT:   {{printf|TGT_PRINTF}}
  // PRT-NOACC-NEXT:   for (l ={{.*}})
  // PRT-NOACC-NEXT:     ;
  // PRT-NOACC-NEXT: }
  #pragma acc loop auto
  for (k = 1; k < 3; ++k) {
    TGT_PRINTF("loop iteration\n");
    for (l = 0; l < 2; ++l)
      ;
  }
  // Each gang will see the old k from before the loop above.
  if (k != 999)
    save->assignLoopFoundBadVal = true;
  // Each gang will see the final l from the loop above.
  if (l != 2)
    save->assignNestedLoopFoundBadVal = true;

  // PRT: k = 999;
  k = 999;
  // k should have been dropped from the loop control variable list, so it
  // should now be implicitly shared in all cases.
  //
  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCAutoClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  // DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
  // DMP-NEXT:   impl: ForStmt
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop auto
  // PRT-AO-SAME: {{^ }}// discarded in OpenMP translation
  //  PRT-A-SAME: {{^$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop auto // discarded in OpenMP translation{{$}}
  //    PRT-NEXT: for (int i = {{.*}}) {
  //    PRT-NEXT:   {{printf|TGT_PRINTF}}
  //    PRT-NEXT:   k = 9999;
  //    PRT-NEXT: }
  #pragma acc loop auto
  for (int i = 0; i < 2; ++i) {
    TGT_PRINTF("loop iteration\n");
    k = 9999;
  }
  // Each gang will see the k from the i loop above.
  if (k != 9999)
    save->writeInLaterLoopFoundBadVal = true;
}

//..............................................................................
// acc loop auto vector
//..............................................................................

// DMP-LABEL: FunctionDecl {{.*}} withinVectorFnAccLoopAutoVectorRun
// PRT-LABEL: static void withinVectorFnAccLoopAutoVectorRun({{([^()]|[[:space:]])*}})
static void withinVectorFnAccLoopAutoVectorRun() {
  int loopOnly;
  int loopOnlyArr[1];
  struct Save save;

  //   EXE-NOT: {{.}}
  // EXE-LABEL: withinVectorFn: acc loop auto vector{{$}}
  //   EXE-NOT: {{.}}
  memset(&save, 0, sizeof(save));
  loopOnly = 88;
  loopOnlyArr[0] = 88;
  printf("withinVectorFn: acc loop auto vector\n");

  // Gang-redundant mode executes each loop iteration per gang.
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  #pragma acc parallel num_gangs(3)
  withinVectorFnAccLoopAutoVector(&save, &loopOnly, loopOnlyArr);

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
}

// DMP-LABEL: FunctionDecl {{.*}} withinVectorFnAccLoopAutoVector
// PRT-LABEL: static void withinVectorFnAccLoopAutoVector({{([^()]|[[:space:]])*}})
static void withinVectorFnAccLoopAutoVector(struct Save *save, int *loopOnly,
                                            int *loopOnlyArr) {
  // PRT: int j = 0;
  int j = 0;
  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCAutoClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCVectorClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  // DMP-NEXT:     DeclRefExpr {{.*}} 'j' 'int'
  // DMP-NEXT:     DeclRefExpr {{.*}} 'save'
  // DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnly' 'int *'
  // DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnlyArr' 'int *'
  // DMP-NEXT:   impl: ForStmt
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop auto vector
  // PRT-AO-SAME: {{^ }}// discarded in OpenMP translation
  //  PRT-A-SAME: {{^$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop auto vector // discarded in OpenMP translation{{$}}
  //    PRT-NEXT: for ({{.*}}) {
  //    PRT-NEXT:   {{printf|TGT_PRINTF}}
  //    PRT-NEXT:   for (j ={{.*}})
  //    PRT-NEXT:     ;
  //         PRT: }
  #pragma acc loop auto vector
  for (int i = 0; i < 2; ++i) {
    TGT_PRINTF("loop iteration\n");
    for (j = 0; j < 2; ++j)
      ;
    checkLoopSeqPrivate(*save, i, loopOnly, *loopOnly, 88, 77);
    *loopOnly = 77;
    checkLoopSeqShared(*save, i, loopOnlyArr, 88, 77);
    loopOnlyArr[0] = 77;
  }
  // Each gang will see the final j from the j loop above.
  if (j != 2)
    save->declNestedLoopFoundBadVal = true;

  //      PRT: int k = 999;
  // PRT-NEXT: int l = 888;
  int k = 999;
  int l = 888;
  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCAutoClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCVectorClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCPrivateClause {{.*}} <predetermined>
  // DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
  // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  // DMP-NEXT:     DeclRefExpr {{.*}} 'l' 'int'
  // DMP-NEXT:   impl: CompoundStmt
  // DMP-NEXT:     DeclStmt
  // DMP-NEXT:       VarDecl {{.*}} k 'int'
  // DMP-NEXT:     ForStmt
  //
  // PRT-AO-NEXT: // v----------ACC----------v
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop auto vector{{$}}
  //  PRT-A-NEXT: for (k ={{.*}}) {
  //  PRT-A-NEXT:   {{print|TGT_PRINTF}}
  //  PRT-A-NEXT:   for (l ={{.*}})
  //  PRT-A-NEXT:     ;
  //  PRT-A-NEXT: }
  // PRT-AO-NEXT: // ---------ACC->OMP--------
  // PRT-AO-NEXT: // {
  // PRT-AO-NEXT: //   int k;
  // PRT-AO-NEXT: //   for (k ={{.*}}) {
  // PRT-AO-NEXT: //     {{printf|TGT_PRINTF}}
  // PRT-AO-NEXT: //     for (l ={{.*}})
  // PRT-AO-NEXT: //       ;
  // PRT-AO-NEXT: //   }
  // PRT-AO-NEXT: // }
  // PRT-AO-NEXT: // ^----------OMP----------^
  //
  // PRT-OA-NEXT: // v----------OMP----------v
  //  PRT-O-NEXT: {
  //  PRT-O-NEXT:   int k;
  //  PRT-O-NEXT:   for (k ={{.*}}) {
  //  PRT-O-NEXT:     {{printf|TGT_PRINTF}}
  //  PRT-O-NEXT:     for (l ={{.*}})
  //  PRT-O-NEXT:       ;
  //  PRT-O-NEXT:   }
  //  PRT-O-NEXT: }
  // PRT-OA-NEXT: // ---------OMP<-ACC--------
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop auto vector{{$}}
  // PRT-OA-NEXT: // for (k ={{.*}}) {
  // PRT-OA-NEXT: //   {{printf|TGT_PRINTF}}
  // PRT-OA-NEXT: //   for (l ={{.*}})
  // PRT-OA-NEXT: //     ;
  // PRT-OA-NEXT: // }
  // PRT-OA-NEXT: // ^----------ACC----------^
  //
  // PRT-NOACC-NEXT: for (k ={{.*}}) {
  // PRT-NOACC-NEXT:   {{printf|TGT_PRINTF}}
  // PRT-NOACC-NEXT:   for (l ={{.*}})
  // PRT-NOACC-NEXT:     ;
  // PRT-NOACC-NEXT: }
  #pragma acc loop auto vector
  for (k = 1; k < 3; ++k) {
    TGT_PRINTF("loop iteration\n");
    for (l = 0; l < 2; ++l)
      ;
  }
  // Each gang will see the old k from before the loop above.
  if (k != 999)
    save->assignLoopFoundBadVal = true;
  // Each gang will see the final l from the loop above.
  if (l != 2)
    save->assignNestedLoopFoundBadVal = true;

  // PRT: k = 999;
  k = 999;
  // k should have been dropped from the loop control variable list, so it
  // should now be implicitly shared in all cases.
  //
  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCAutoClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCVectorClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  // DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
  // DMP-NEXT:   impl: ForStmt
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop auto vector
  // PRT-AO-SAME: {{^ }}// discarded in OpenMP translation
  //  PRT-A-SAME: {{^$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop auto vector // discarded in OpenMP translation{{$}}
  //    PRT-NEXT: for (int i = {{.*}}) {
  //    PRT-NEXT:   {{printf|TGT_PRINTF}}
  //    PRT-NEXT:   k = 9999;
  //    PRT-NEXT: }
  #pragma acc loop auto vector
  for (int i = 0; i < 2; ++i) {
    TGT_PRINTF("loop iteration\n");
    k = 9999;
  }
  // Each gang will see the k from the i loop above.
  if (k != 9999)
    save->writeInLaterLoopFoundBadVal = true;
}

//..............................................................................
// acc loop INDEPENDENT
//..............................................................................

// DMP-LABEL: FunctionDecl {{.*}} withinVectorFnAccLoopIndependentRun
// PRT-LABEL: static void withinVectorFnAccLoopIndependentRun({{([^()]|[[:space:]])*}})
static void withinVectorFnAccLoopIndependentRun() {
  int loopOnly;
  int loopOnlyArr[1];
  struct Save save;

  //   EXE-NOT: {{.}}
  // EXE-LABEL: withinVectorFn: acc loop{{$}}
  //   EXE-NOT: {{.}}
  memset(&save, 0, sizeof(save));
  loopOnly = 88;
  loopOnlyArr[0] = 88;
  printf("withinVectorFn: acc loop\n");

  // Gang-redundant mode executes each loop iteration per gang.
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  #pragma acc parallel num_gangs(3)
  withinVectorFnAccLoopIndependent(&save, &loopOnly, loopOnlyArr);

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
}

// DMP-LABEL: FunctionDecl {{.*}} withinVectorFnAccLoopIndependent
// PRT-LABEL: static void withinVectorFnAccLoopIndependent({{([^()]|[[:space:]])*}})
static void withinVectorFnAccLoopIndependent(struct Save *save, int *loopOnly,
                                             int *loopOnlyArr) {
  // PRT: int j = 0;
  int j = 0;
  //             DMP: ACCLoopDirective
  //    DMP-IND-NEXT:   ACCIndependentClause
  //     DMP-IND-NOT:     <implicit>
  // DMP-NO-IND-NEXT:   ACCIndependentClause {{.*}} <implicit>
  //        DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  //        DMP-NEXT:     DeclRefExpr {{.*}} 'j' 'int'
  //        DMP-NEXT:     DeclRefExpr {{.*}} 'save'
  //        DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnly' 'int *'
  //        DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnlyArr' 'int *'
  //        DMP-NEXT:   impl: ForStmt
  //
  //          PRT-A-NEXT: {{^ *}}#pragma acc loop
  //      PRT-A-SRC-SAME: {{^ }}INDEPENDENT
  //  PRT-A-AST-IND-SAME: {{^ }}independent
  //         PRT-AO-SAME: {{^ }}// discarded in OpenMP translation
  //          PRT-A-SAME: {{^$}}
  //         PRT-OA-NEXT: {{^ *}}// #pragma acc loop
  //     PRT-OA-SRC-SAME: {{^ }}INDEPENDENT
  // PRT-OA-AST-IND-SAME: {{^ }}independent
  //         PRT-OA-SAME: {{^ }}// discarded in OpenMP translation{{$}}
  //            PRT-NEXT: for ({{.*}}) {
  //            PRT-NEXT:   {{printf|TGT_PRINTF}}
  //            PRT-NEXT:   for (j ={{.*}})
  //            PRT-NEXT:     ;
  //                 PRT: }
  #pragma acc loop INDEPENDENT
  for (int i = 0; i < 2; ++i) {
    TGT_PRINTF("loop iteration\n");
    for (j = 0; j < 2; ++j)
      ;
    checkLoopSeqPrivate(*save, i, loopOnly, *loopOnly, 88, 77);
    *loopOnly = 77;
    checkLoopSeqShared(*save, i, loopOnlyArr, 88, 77);
    loopOnlyArr[0] = 77;
  }
  // Each gang will see the final j from the j loop above.
  if (j != 2)
    save->declNestedLoopFoundBadVal = true;

  //      PRT: int k = 999;
  // PRT-NEXT: int l = 888;
  int k = 999;
  int l = 888;
  //             DMP: ACCLoopDirective
  //    DMP-IND-NEXT:   ACCIndependentClause
  //     DMP-IND-NOT:     <implicit>
  // DMP-NO-IND-NEXT:   ACCIndependentClause {{.*}} <implicit>
  //        DMP-NEXT:   ACCPrivateClause {{.*}} <predetermined>
  //        DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
  //        DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  //        DMP-NEXT:     DeclRefExpr {{.*}} 'l' 'int'
  //        DMP-NEXT:   impl: CompoundStmt
  //        DMP-NEXT:     DeclStmt
  //        DMP-NEXT:       VarDecl {{.*}} k 'int'
  //        DMP-NEXT:     ForStmt
  //
  //        PRT-AO-NEXT: // v----------ACC----------v
  //         PRT-A-NEXT: {{^ *}}#pragma acc loop
  //     PRT-A-SRC-SAME: {{^ }}INDEPENDENT
  // PRT-A-AST-IND-SAME: {{^ }}independent
  //         PRT-A-SAME: {{^$}}
  //         PRT-A-NEXT: for (k ={{.*}}) {
  //         PRT-A-NEXT:   {{print|TGT_PRINTF}}
  //         PRT-A-NEXT:   for (l ={{.*}})
  //         PRT-A-NEXT:     ;
  //         PRT-A-NEXT: }
  //        PRT-AO-NEXT: // ---------ACC->OMP--------
  //        PRT-AO-NEXT: // {
  //        PRT-AO-NEXT: //   int k;
  //        PRT-AO-NEXT: //   for (k ={{.*}}) {
  //        PRT-AO-NEXT: //     {{printf|TGT_PRINTF}}
  //        PRT-AO-NEXT: //     for (l ={{.*}})
  //        PRT-AO-NEXT: //       ;
  //        PRT-AO-NEXT: //   }
  //        PRT-AO-NEXT: // }
  //        PRT-AO-NEXT: // ^----------OMP----------^
  //
  //         PRT-OA-NEXT: // v----------OMP----------v
  //          PRT-O-NEXT: {
  //          PRT-O-NEXT:   int k;
  //          PRT-O-NEXT:   for (k ={{.*}}) {
  //          PRT-O-NEXT:     {{printf|TGT_PRINTF}}
  //          PRT-O-NEXT:     for (l ={{.*}})
  //          PRT-O-NEXT:       ;
  //          PRT-O-NEXT:   }
  //          PRT-O-NEXT: }
  //         PRT-OA-NEXT: // ---------OMP<-ACC--------
  //         PRT-OA-NEXT: {{^ *}}// #pragma acc loop
  //     PRT-OA-SRC-SAME: {{^ }}INDEPENDENT
  // PRT-OA-AST-IND-SAME: {{^ }}independent
  //         PRT-OA-SAME: {{^$}}
  //         PRT-OA-NEXT: // for (k ={{.*}}) {
  //         PRT-OA-NEXT: //   {{printf|TGT_PRINTF}}
  //         PRT-OA-NEXT: //   for (l ={{.*}})
  //         PRT-OA-NEXT: //     ;
  //         PRT-OA-NEXT: // }
  //         PRT-OA-NEXT: // ^----------ACC----------^
  //
  // PRT-NOACC-NEXT: for (k ={{.*}}) {
  // PRT-NOACC-NEXT:   {{printf|TGT_PRINTF}}
  // PRT-NOACC-NEXT:   for (l ={{.*}})
  // PRT-NOACC-NEXT:     ;
  // PRT-NOACC-NEXT: }
  #pragma acc loop INDEPENDENT
  for (k = 1; k < 3; ++k) {
    TGT_PRINTF("loop iteration\n");
    for (l = 0; l < 2; ++l)
      ;
  }
  // Each gang will see the old k from before the loop above.
  if (k != 999)
    save->assignLoopFoundBadVal = true;
  // Each gang will see the final l from the loop above.
  if (l != 2)
    save->assignNestedLoopFoundBadVal = true;

  // PRT: k = 999;
  k = 999;
  // k should have been dropped from the loop control variable list, so it
  // should now be implicitly shared in all cases.
  //
  //             DMP: ACCLoopDirective
  //    DMP-IND-NEXT:   ACCIndependentClause
  //     DMP-IND-NOT:     <implicit>
  // DMP-NO-IND-NEXT:   ACCIndependentClause {{.*}} <implicit>
  //        DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  //        DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
  //        DMP-NEXT:   impl: ForStmt
  //
  //          PRT-A-NEXT: {{^ *}}#pragma acc loop
  //      PRT-A-SRC-SAME: {{^ }}INDEPENDENT
  //  PRT-A-AST-IND-SAME: {{^ }}independent
  //         PRT-AO-SAME: {{^ }}// discarded in OpenMP translation
  //          PRT-A-SAME: {{^$}}
  //         PRT-OA-NEXT: {{^ *}}// #pragma acc loop
  //     PRT-OA-SRC-SAME: {{^ }}INDEPENDENT
  // PRT-OA-AST-IND-SAME: {{^ }}independent
  //         PRT-OA-SAME: {{^ }}// discarded in OpenMP translation{{$}}
  //            PRT-NEXT: for (int i = {{.*}}) {
  //            PRT-NEXT:   {{printf|TGT_PRINTF}}
  //            PRT-NEXT:   k = 9999;
  //            PRT-NEXT: }
  #pragma acc loop INDEPENDENT
  for (int i = 0; i < 2; ++i) {
    TGT_PRINTF("loop iteration\n");
    k = 9999;
  }
  // Each gang will see the k from the i loop above.
  if (k != 9999)
    save->writeInLaterLoopFoundBadVal = true;
}

//------------------------------------------------------------------------------
// acc loop INDEPENDENT vector
//------------------------------------------------------------------------------

// DMP-LABEL: FunctionDecl {{.*}} withinVectorFnAccLoopIndependentVectorRun
// PRT-LABEL: static void withinVectorFnAccLoopIndependentVectorRun({{([^()]|[[:space:]])*}})
static void withinVectorFnAccLoopIndependentVectorRun() {
  int loopOnly;
  int loopOnlyArr[1];
  struct Save save;

  //   EXE-NOT: {{.}}
  // EXE-LABEL: withinVectorFn: acc loop INDEPENDENT vector{{$}}
  //   EXE-NOT: {{.}}
  memset(&save, 0, sizeof(save));
  loopOnly = 88;
  loopOnlyArr[0] = 88;
  printf("withinVectorFn: acc loop INDEPENDENT vector\n");

  // Call from a gang loop: Gang-partitioned mode executes each outer loop
  // iteration once.
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  #pragma acc parallel num_gangs(3)
  #pragma acc loop gang
  for (int g = 0; g < 2; ++g)
    withinVectorFnAccLoopIndependentVector(&save, &loopOnly, loopOnlyArr);
  // EXE-NEXT: save.loopOnlyFoundOldVal=1
  // EXE-NEXT: save.loopOnlyFoundBadVal=0
  // EXE-NEXT: save.loopOnlyArrFoundOldVal=1
  // EXE-NEXT: save.loopOnlyArrFoundBadVal=0
  // EXE-NEXT: save.assignLoopFoundBadVal=0
  // EXE-NEXT: save.writeInLaterLoopFoundBadVal=0
  // EXE-NEXT: loopOnly=88
  // EXE-NEXT: loopOnlyArr[0]=77
  printf("save.loopOnlyFoundOldVal=%d\n", save.loopOnlyFoundOldVal);
  printf("save.loopOnlyFoundBadVal=%d\n", save.loopOnlyFoundBadVal);
  printf("save.loopOnlyArrFoundOldVal=%d\n", save.loopOnlyArrFoundOldVal);
  printf("save.loopOnlyArrFoundBadVal=%d\n", save.loopOnlyArrFoundBadVal);
  printf("save.assignLoopFoundBadVal=%d\n", save.assignLoopFoundBadVal);
  printf("save.writeInLaterLoopFoundBadVal=%d\n", save.writeInLaterLoopFoundBadVal);
  printf("loopOnly=%d\n", loopOnly);
  printf("loopOnlyArr[0]=%d\n", loopOnlyArr[0]);

  // Call from a gang/worker loop: Gang-partitioned mode executes each outer
  // loop iteration once.
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  #pragma acc parallel num_gangs(3)
  #pragma acc loop gang worker
  for (int gw = 0; gw < 2; ++gw)
    withinVectorFnAccLoopIndependentVector(&save, &loopOnly, loopOnlyArr);
  // EXE-NEXT: save.loopOnlyFoundOldVal=1
  // EXE-NEXT: save.loopOnlyFoundBadVal=0
  // EXE-NEXT: save.loopOnlyArrFoundOldVal=1
  // EXE-NEXT: save.loopOnlyArrFoundBadVal=0
  // EXE-NEXT: save.assignLoopFoundBadVal=0
  // EXE-NEXT: save.writeInLaterLoopFoundBadVal=0
  // EXE-NEXT: loopOnly=88
  // EXE-NEXT: loopOnlyArr[0]=77
  printf("save.loopOnlyFoundOldVal=%d\n", save.loopOnlyFoundOldVal);
  printf("save.loopOnlyFoundBadVal=%d\n", save.loopOnlyFoundBadVal);
  printf("save.loopOnlyArrFoundOldVal=%d\n", save.loopOnlyArrFoundOldVal);
  printf("save.loopOnlyArrFoundBadVal=%d\n", save.loopOnlyArrFoundBadVal);
  printf("save.assignLoopFoundBadVal=%d\n", save.assignLoopFoundBadVal);
  printf("save.writeInLaterLoopFoundBadVal=%d\n", save.writeInLaterLoopFoundBadVal);
  printf("loopOnly=%d\n", loopOnly);
  printf("loopOnlyArr[0]=%d\n", loopOnlyArr[0]);

  // Call directly from the parallel construct: Gang-redundant mode executes the
  // function per gang.  Orphaned loops are the only way to achieve this
  // gang-redundant, vector-partitioned mode.
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  #pragma acc parallel num_gangs(3)
  withinVectorFnAccLoopIndependentVector(&save, &loopOnly, loopOnlyArr);
  // EXE-NEXT: save.loopOnlyFoundOldVal=1
  // EXE-NEXT: save.loopOnlyFoundBadVal=0
  // EXE-NEXT: save.loopOnlyArrFoundOldVal=1
  // EXE-NEXT: save.loopOnlyArrFoundBadVal=0
  // EXE-NEXT: save.assignLoopFoundBadVal=0
  // EXE-NEXT: save.writeInLaterLoopFoundBadVal=0
  // EXE-NEXT: loopOnly=88
  // EXE-NEXT: loopOnlyArr[0]=77
  printf("save.loopOnlyFoundOldVal=%d\n", save.loopOnlyFoundOldVal);
  printf("save.loopOnlyFoundBadVal=%d\n", save.loopOnlyFoundBadVal);
  printf("save.loopOnlyArrFoundOldVal=%d\n", save.loopOnlyArrFoundOldVal);
  printf("save.loopOnlyArrFoundBadVal=%d\n", save.loopOnlyArrFoundBadVal);
  printf("save.assignLoopFoundBadVal=%d\n", save.assignLoopFoundBadVal);
  printf("save.writeInLaterLoopFoundBadVal=%d\n", save.writeInLaterLoopFoundBadVal);
  printf("loopOnly=%d\n", loopOnly);
  printf("loopOnlyArr[0]=%d\n", loopOnlyArr[0]);
}

// DMP-LABEL: FunctionDecl {{.*}} withinVectorFnAccLoopIndependentVector
// PRT-LABEL: static void withinVectorFnAccLoopIndependentVector({{([^()]|[[:space:]])*}})
static void withinVectorFnAccLoopIndependentVector(struct Save *save,
                                                   int *loopOnly,
                                                   int *loopOnlyArr) {
  //             DMP: ACCLoopDirective
  //    DMP-IND-NEXT:   ACCIndependentClause
  //     DMP-IND-NOT:     <implicit>
  //        DMP-NEXT:   ACCVectorClause
  //         DMP-NOT:     <implicit>
  // DMP-NO-IND-NEXT:   ACCIndependentClause {{.*}} <implicit>
  //        DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  //        DMP-NEXT:     DeclRefExpr {{.*}} 'save'
  //        DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnly' 'int *'
  //        DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnlyArr' 'int *'
  //        DMP-NEXT:   impl: OMPSimdDirective
  //         DMP-NOT:     OMP
  //             DMP:     ForStmt
  //
  //          PRT-A-NEXT: {{^ *}}#pragma acc loop
  //      PRT-A-SRC-SAME: {{^ }}INDEPENDENT
  //  PRT-A-AST-IND-SAME: {{^ }}independent
  //          PRT-A-SAME: {{^ }}vector{{$}}
  //         PRT-AO-NEXT: {{^ *}}// #pragma omp simd{{$}}
  //          PRT-O-NEXT: {{^ *}}#pragma omp simd{{$}}
  //         PRT-OA-NEXT: {{^ *}}// #pragma acc loop
  //     PRT-OA-SRC-SAME: {{^ }}INDEPENDENT
  // PRT-OA-AST-IND-SAME: {{^ }}independent
  //         PRT-OA-SAME: {{^ }}vector{{$}}
  //            PRT-NEXT: for (int i = {{.*}}) {
  //            PRT-NEXT:   {{printf|TGT_PRINTF}}
  //                 PRT: }
  #pragma acc loop INDEPENDENT vector
  for (int i = 0; i < 2; ++i) {
    TGT_PRINTF("loop iteration\n");
    // We omit checking of an inner j loop this time.  j would be shared
    // among vector lanes and thus would have an unpredictable value.
    checkLoopPartShared(*save, i, loopOnly, *loopOnly, 88, 77);
    *loopOnly = 77;
    checkLoopPartShared(*save, i, loopOnlyArr, loopOnlyArr[0], 88, 77);
    loopOnlyArr[0] = 77;
  }

  // PRT: int k = 999;
  int k = 999;
  //              DMP: ACCLoopDirective
  //     DMP-IND-NEXT:   ACCIndependentClause
  //      DMP-IND-NOT:     <implicit>
  //         DMP-NEXT:   ACCVectorClause
  //          DMP-NOT:     <implicit>
  //  DMP-NO-IND-NEXT:   ACCIndependentClause {{.*}} <implicit>
  //         DMP-NEXT:   ACCPrivateClause {{.*}} <predetermined>
  //         DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
  //         DMP-NEXT:   impl: CompoundStmt
  //         DMP-NEXT:     DeclStmt
  //         DMP-NEXT:       VarDecl {{.*}} k 'int'
  //         DMP-NEXT:     OMPSimdDirective
  //          DMP-NOT:       OMPPrivateClause
  //              DMP:       ForStmt
  //
  //         PRT-AO-NEXT: // v----------ACC----------v
  //          PRT-A-NEXT: {{^ *}}#pragma acc loop
  //      PRT-A-SRC-SAME: {{^ }}INDEPENDENT
  //  PRT-A-AST-IND-SAME: {{^ }}independent
  //          PRT-A-SAME: {{^ }}vector{{$}}
  //          PRT-A-NEXT: for (k ={{.*}}) {
  //          PRT-A-NEXT:   {{print|TGT_PRINTF}}
  //          PRT-A-NEXT: }
  //         PRT-AO-NEXT: // ---------ACC->OMP--------
  //         PRT-AO-NEXT: // {
  //         PRT-AO-NEXT: //   int k;
  //         PRT-AO-NEXT: //   #pragma omp simd{{$}}
  //         PRT-AO-NEXT: //   for (k ={{.*}}) {
  //         PRT-AO-NEXT: //     {{printf|TGT_PRINTF}}
  //         PRT-AO-NEXT: //   }
  //         PRT-AO-NEXT: // }
  //         PRT-AO-NEXT: // ^----------OMP----------^
  //
  //         PRT-OA-NEXT: // v----------OMP----------v
  //          PRT-O-NEXT: {
  //          PRT-O-NEXT:   int k;
  //          PRT-O-NEXT:   {{^ *}}#pragma omp simd{{$}}
  //          PRT-O-NEXT:   for (k ={{.*}}) {
  //          PRT-O-NEXT:     {{printf|TGT_PRINTF}}
  //          PRT-O-NEXT:   }
  //          PRT-O-NEXT: }
  //         PRT-OA-NEXT: // ---------OMP<-ACC--------
  //         PRT-OA-NEXT: {{^ *}}// #pragma acc loop
  //     PRT-OA-SRC-SAME: {{^ }}INDEPENDENT
  // PRT-OA-AST-IND-SAME: {{^ }}independent
  //         PRT-OA-SAME: {{^ }}vector{{$}}
  //         PRT-OA-NEXT: // for (k ={{.*}}) {
  //         PRT-OA-NEXT: //   {{printf|TGT_PRINTF}}
  //         PRT-OA-NEXT: // }
  //         PRT-OA-NEXT: // ^----------ACC----------^
  //
  // PRT-NOACC-NEXT: for (k ={{.*}}) {
  // PRT-NOACC-NEXT:   {{printf|TGT_PRINTF}}
  // PRT-NOACC-NEXT: }
  #pragma acc loop INDEPENDENT vector
  for (k = 1; k < 3; ++k) {
    TGT_PRINTF("loop iteration\n");
    // We omit checking of an inner l loop this time.  l would be shared
    // among vector lanes and thus would have an unpredictable value.
  }
  // Each gang will see the old k from before the loop above.
  if (k != 999)
    save->assignLoopFoundBadVal = true;

  // PRT: k = 999;
  k = 999;
  // k should have been dropped from the loop control variable list, so it
  // should now be implicitly shared in all cases.
  //
  //             DMP: ACCLoopDirective
  //    DMP-IND-NEXT:   ACCIndependentClause
  //     DMP-IND-NOT:     <implicit>
  //        DMP-NEXT:   ACCVectorClause
  //         DMP-NOT:     <implicit>
  // DMP-NO-IND-NEXT:   ACCIndependentClause {{.*}} <implicit>
  //        DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  //        DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
  //        DMP-NEXT:   impl: OMPSimdDirective
  //         DMP-NOT:     OMP
  //             DMP:     ForStmt
  //
  //          PRT-A-NEXT: {{^ *}}#pragma acc loop
  //      PRT-A-SRC-SAME: {{^ }}INDEPENDENT
  //  PRT-A-AST-IND-SAME: {{^ }}independent
  //          PRT-A-SAME: {{^ }}vector{{$}}
  //         PRT-AO-NEXT: {{^ *}}// #pragma omp simd{{$}}
  //          PRT-O-NEXT: {{^ *}}#pragma omp simd{{$}}
  //         PRT-OA-NEXT: {{^ *}}// #pragma acc loop
  //     PRT-OA-SRC-SAME: {{^ }}INDEPENDENT
  // PRT-OA-AST-IND-SAME: {{^ }}independent
  //         PRT-OA-SAME: {{^ }}vector{{$}}
  //            PRT-NEXT: for ({{.*}}) {
  //            PRT-NEXT:   {{printf|TGT_PRINTF}}
  //            PRT-NEXT:   k = 9999;
  //            PRT-NEXT: }
  #pragma acc loop INDEPENDENT vector
  for (int i = 0; i < 2; ++i) {
    TGT_PRINTF("loop iteration\n");
    k = 9999;
  }
  // Each gang that calls this function will see the k from the i loop above.
  if (k != 9999)
    save->writeInLaterLoopFoundBadVal = true;
}

//==============================================================================
// Check within seq functions.
//
// This repeats the above testing but within a seq function.
//==============================================================================

//------------------------------------------------------------------------------
// acc loop seq
//------------------------------------------------------------------------------

// DMP-LABEL: FunctionDecl {{.*}} withinSeqFnAccLoopSeqRun
// PRT-LABEL: static void withinSeqFnAccLoopSeqRun({{([^()]|[[:space:]])*}})
static void withinSeqFnAccLoopSeqRun() {
  int loopOnly;
  int loopOnlyArr[1];
  struct Save save;

  //   EXE-NOT: {{.}}
  // EXE-LABEL: withinSeqFn: acc loop seq{{$}}
  //   EXE-NOT: {{.}}
  memset(&save, 0, sizeof(save));
  loopOnly = 88;
  loopOnlyArr[0] = 88;
  printf("withinSeqFn: acc loop seq\n");

  // Gang-redundant mode executes each loop iteration per gang.
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  #pragma acc parallel num_gangs(3)
  withinSeqFnAccLoopSeq(&save, &loopOnly, loopOnlyArr);
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
}

// DMP-LABEL: FunctionDecl {{.*}} withinSeqFnAccLoopSeq
// PRT-LABEL: static void withinSeqFnAccLoopSeq({{([^()]|[[:space:]])*}})
static void withinSeqFnAccLoopSeq(struct Save *save, int *loopOnly,
                                  int *loopOnlyArr) {
  // PRT: int j = 0;
  int j = 0;
  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCSeqClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  // DMP-NEXT:     DeclRefExpr {{.*}} 'j' 'int'
  // DMP-NEXT:     DeclRefExpr {{.*}} 'save'
  // DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnly' 'int *'
  // DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnlyArr' 'int *'
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
  #pragma acc loop seq
  for (int i = 0; i < 2; ++i) {
    TGT_PRINTF("loop iteration\n");
    for (j = 0; j < 2; ++j)
      ;
    checkLoopSeqPrivate(*save, i, loopOnly, *loopOnly, 88, 77);
    *loopOnly = 77;
    checkLoopSeqShared(*save, i, loopOnlyArr, 88, 77);
    loopOnlyArr[0] = 77;
  }
  // Each gang will see the final j from the j loop above.
  if (j != 2)
    save->declNestedLoopFoundBadVal = true;

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
  #pragma acc loop seq
  for (k = 1; k < 3; ++k) {
    TGT_PRINTF("loop iteration\n");
    for (l = 0; l < 2; ++l)
      ;
  }
  // Each gang will see the final k and l from the loop above.
  if (k != 3)
    save->assignLoopFoundBadVal = true;
  if (l != 2)
    save->assignNestedLoopFoundBadVal = true;
}

//------------------------------------------------------------------------------
// acc loop auto
//------------------------------------------------------------------------------

// DMP-LABEL: FunctionDecl {{.*}} withinSeqFnAccLoopAutoRun
// PRT-LABEL: static void withinSeqFnAccLoopAutoRun({{([^()]|[[:space:]])*}})
static void withinSeqFnAccLoopAutoRun() {
  int loopOnly;
  int loopOnlyArr[1];
  struct Save save;

  //   EXE-NOT: {{.}}
  // EXE-LABEL: withinSeqFn: acc loop auto{{$}}
  //   EXE-NOT: {{.}}
  memset(&save, 0, sizeof(save));
  loopOnly = 88;
  loopOnlyArr[0] = 88;
  printf("withinSeqFn: acc loop auto\n");

  // Gang-redundant mode executes each loop iteration per gang.
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  #pragma acc parallel num_gangs(3)
  withinSeqFnAccLoopAuto(&save, &loopOnly, loopOnlyArr);

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
}

// DMP-LABEL: FunctionDecl {{.*}} withinSeqFnAccLoopAuto
// PRT-LABEL: static void withinSeqFnAccLoopAuto({{([^()]|[[:space:]])*}})
static void withinSeqFnAccLoopAuto(struct Save *save, int *loopOnly,
                                   int *loopOnlyArr) {
  // PRT: int j = 0;
  int j = 0;
  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCAutoClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  // DMP-NEXT:     DeclRefExpr {{.*}} 'j' 'int'
  // DMP-NEXT:     DeclRefExpr {{.*}} 'save'
  // DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnly' 'int *'
  // DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnlyArr' 'int *'
  // DMP-NEXT:   impl: ForStmt
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop auto
  // PRT-AO-SAME: {{^ }}// discarded in OpenMP translation
  //  PRT-A-SAME: {{^$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop auto // discarded in OpenMP translation{{$}}
  //    PRT-NEXT: for ({{.*}}) {
  //    PRT-NEXT:   {{printf|TGT_PRINTF}}
  //    PRT-NEXT:   for (j ={{.*}})
  //    PRT-NEXT:     ;
  //         PRT: }
  #pragma acc loop auto
  for (int i = 0; i < 2; ++i) {
    TGT_PRINTF("loop iteration\n");
    for (j = 0; j < 2; ++j)
      ;
    checkLoopSeqPrivate(*save, i, loopOnly, *loopOnly, 88, 77);
    *loopOnly = 77;
    checkLoopSeqShared(*save, i, loopOnlyArr, 88, 77);
    loopOnlyArr[0] = 77;
  }
  // Each gang will see the final j from the j loop above.
  if (j != 2)
    save->declNestedLoopFoundBadVal = true;

  //      PRT: int k = 999;
  // PRT-NEXT: int l = 888;
  int k = 999;
  int l = 888;
  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCAutoClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCPrivateClause {{.*}} <predetermined>
  // DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
  // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  // DMP-NEXT:     DeclRefExpr {{.*}} 'l' 'int'
  // DMP-NEXT:   impl: CompoundStmt
  // DMP-NEXT:     DeclStmt
  // DMP-NEXT:       VarDecl {{.*}} k 'int'
  // DMP-NEXT:     ForStmt
  //
  // PRT-AO-NEXT: // v----------ACC----------v
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop auto{{$}}
  //  PRT-A-NEXT: for (k ={{.*}}) {
  //  PRT-A-NEXT:   {{print|TGT_PRINTF}}
  //  PRT-A-NEXT:   for (l ={{.*}})
  //  PRT-A-NEXT:     ;
  //  PRT-A-NEXT: }
  // PRT-AO-NEXT: // ---------ACC->OMP--------
  // PRT-AO-NEXT: // {
  // PRT-AO-NEXT: //   int k;
  // PRT-AO-NEXT: //   for (k ={{.*}}) {
  // PRT-AO-NEXT: //     {{printf|TGT_PRINTF}}
  // PRT-AO-NEXT: //     for (l ={{.*}})
  // PRT-AO-NEXT: //       ;
  // PRT-AO-NEXT: //   }
  // PRT-AO-NEXT: // }
  // PRT-AO-NEXT: // ^----------OMP----------^
  //
  // PRT-OA-NEXT: // v----------OMP----------v
  //  PRT-O-NEXT: {
  //  PRT-O-NEXT:   int k;
  //  PRT-O-NEXT:   for (k ={{.*}}) {
  //  PRT-O-NEXT:     {{printf|TGT_PRINTF}}
  //  PRT-O-NEXT:     for (l ={{.*}})
  //  PRT-O-NEXT:       ;
  //  PRT-O-NEXT:   }
  //  PRT-O-NEXT: }
  // PRT-OA-NEXT: // ---------OMP<-ACC--------
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop auto{{$}}
  // PRT-OA-NEXT: // for (k ={{.*}}) {
  // PRT-OA-NEXT: //   {{printf|TGT_PRINTF}}
  // PRT-OA-NEXT: //   for (l ={{.*}})
  // PRT-OA-NEXT: //     ;
  // PRT-OA-NEXT: // }
  // PRT-OA-NEXT: // ^----------ACC----------^
  //
  // PRT-NOACC-NEXT: for (k ={{.*}}) {
  // PRT-NOACC-NEXT:   {{printf|TGT_PRINTF}}
  // PRT-NOACC-NEXT:   for (l ={{.*}})
  // PRT-NOACC-NEXT:     ;
  // PRT-NOACC-NEXT: }
  #pragma acc loop auto
  for (k = 1; k < 3; ++k) {
    TGT_PRINTF("loop iteration\n");
    for (l = 0; l < 2; ++l)
      ;
  }
  // Each gang will see the old k from before the loop above.
  if (k != 999)
    save->assignLoopFoundBadVal = true;
  // Each gang will see the final l from the loop above.
  if (l != 2)
    save->assignNestedLoopFoundBadVal = true;

  // PRT: k = 999;
  k = 999;
  // k should have been dropped from the loop control variable list, so it
  // should now be implicitly shared in all cases.
  //
  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCAutoClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  // DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
  // DMP-NEXT:   impl: ForStmt
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop auto
  // PRT-AO-SAME: {{^ }}// discarded in OpenMP translation
  //  PRT-A-SAME: {{^$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop auto // discarded in OpenMP translation{{$}}
  //    PRT-NEXT: for (int i = {{.*}}) {
  //    PRT-NEXT:   {{printf|TGT_PRINTF}}
  //    PRT-NEXT:   k = 9999;
  //    PRT-NEXT: }
  #pragma acc loop auto
  for (int i = 0; i < 2; ++i) {
    TGT_PRINTF("loop iteration\n");
    k = 9999;
  }
  // Each gang will see the k from the i loop above.
  if (k != 9999)
    save->writeInLaterLoopFoundBadVal = true;
}

//------------------------------------------------------------------------------
// acc loop INDEPENDENT
//------------------------------------------------------------------------------

// DMP-LABEL: FunctionDecl {{.*}} withinSeqFnAccLoopIndependentRun
// PRT-LABEL: static void withinSeqFnAccLoopIndependentRun({{([^()]|[[:space:]])*}})
static void withinSeqFnAccLoopIndependentRun() {
  int loopOnly;
  int loopOnlyArr[1];
  struct Save save;

  //   EXE-NOT: {{.}}
  // EXE-LABEL: withinSeqFn: acc loop{{$}}
  //   EXE-NOT: {{.}}
  memset(&save, 0, sizeof(save));
  loopOnly = 88;
  loopOnlyArr[0] = 88;
  printf("withinSeqFn: acc loop\n");

  // Gang-redundant mode executes each loop iteration per gang.
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  #pragma acc parallel num_gangs(3)
  withinSeqFnAccLoopIndependent(&save, &loopOnly, loopOnlyArr);

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
}

// DMP-LABEL: FunctionDecl {{.*}} withinSeqFnAccLoopIndependent
// PRT-LABEL: static void withinSeqFnAccLoopIndependent({{([^()]|[[:space:]])*}})
static void withinSeqFnAccLoopIndependent(struct Save *save, int *loopOnly,
                                          int *loopOnlyArr) {
  // PRT: int j = 0;
  int j = 0;
  //             DMP: ACCLoopDirective
  //    DMP-IND-NEXT:   ACCIndependentClause
  //     DMP-IND-NOT:     <implicit>
  // DMP-NO-IND-NEXT:   ACCIndependentClause {{.*}} <implicit>
  //        DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  //        DMP-NEXT:     DeclRefExpr {{.*}} 'j' 'int'
  //        DMP-NEXT:     DeclRefExpr {{.*}} 'save'
  //        DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnly' 'int *'
  //        DMP-NEXT:     DeclRefExpr {{.*}} 'loopOnlyArr' 'int *'
  //        DMP-NEXT:   impl: ForStmt
  //
  //          PRT-A-NEXT: {{^ *}}#pragma acc loop
  //      PRT-A-SRC-SAME: {{^ }}INDEPENDENT
  //  PRT-A-AST-IND-SAME: {{^ }}independent
  //         PRT-AO-SAME: {{^ }}// discarded in OpenMP translation
  //          PRT-A-SAME: {{^$}}
  //         PRT-OA-NEXT: {{^ *}}// #pragma acc loop
  //     PRT-OA-SRC-SAME: {{^ }}INDEPENDENT
  // PRT-OA-AST-IND-SAME: {{^ }}independent
  //         PRT-OA-SAME: {{^ }}// discarded in OpenMP translation{{$}}
  //            PRT-NEXT: for ({{.*}}) {
  //            PRT-NEXT:   {{printf|TGT_PRINTF}}
  //            PRT-NEXT:   for (j ={{.*}})
  //            PRT-NEXT:     ;
  //                 PRT: }
  #pragma acc loop INDEPENDENT
  for (int i = 0; i < 2; ++i) {
    TGT_PRINTF("loop iteration\n");
    for (j = 0; j < 2; ++j)
      ;
    checkLoopSeqPrivate(*save, i, loopOnly, *loopOnly, 88, 77);
    *loopOnly = 77;
    checkLoopSeqShared(*save, i, loopOnlyArr, 88, 77);
    loopOnlyArr[0] = 77;
  }
  // Each gang will see the final j from the j loop above.
  if (j != 2)
    save->declNestedLoopFoundBadVal = true;

  //      PRT: int k = 999;
  // PRT-NEXT: int l = 888;
  int k = 999;
  int l = 888;
  //             DMP: ACCLoopDirective
  //    DMP-IND-NEXT:   ACCIndependentClause
  //     DMP-IND-NOT:     <implicit>
  // DMP-NO-IND-NEXT:   ACCIndependentClause {{.*}} <implicit>
  //        DMP-NEXT:   ACCPrivateClause {{.*}} <predetermined>
  //        DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
  //        DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  //        DMP-NEXT:     DeclRefExpr {{.*}} 'l' 'int'
  //        DMP-NEXT:   impl: CompoundStmt
  //        DMP-NEXT:     DeclStmt
  //        DMP-NEXT:       VarDecl {{.*}} k 'int'
  //        DMP-NEXT:     ForStmt
  //
  //        PRT-AO-NEXT: // v----------ACC----------v
  //         PRT-A-NEXT: {{^ *}}#pragma acc loop
  //     PRT-A-SRC-SAME: {{^ }}INDEPENDENT
  // PRT-A-AST-IND-SAME: {{^ }}independent
  //         PRT-A-SAME: {{^$}}
  //         PRT-A-NEXT: for (k ={{.*}}) {
  //         PRT-A-NEXT:   {{print|TGT_PRINTF}}
  //         PRT-A-NEXT:   for (l ={{.*}})
  //         PRT-A-NEXT:     ;
  //         PRT-A-NEXT: }
  //        PRT-AO-NEXT: // ---------ACC->OMP--------
  //        PRT-AO-NEXT: // {
  //        PRT-AO-NEXT: //   int k;
  //        PRT-AO-NEXT: //   for (k ={{.*}}) {
  //        PRT-AO-NEXT: //     {{printf|TGT_PRINTF}}
  //        PRT-AO-NEXT: //     for (l ={{.*}})
  //        PRT-AO-NEXT: //       ;
  //        PRT-AO-NEXT: //   }
  //        PRT-AO-NEXT: // }
  //        PRT-AO-NEXT: // ^----------OMP----------^
  //
  //         PRT-OA-NEXT: // v----------OMP----------v
  //          PRT-O-NEXT: {
  //          PRT-O-NEXT:   int k;
  //          PRT-O-NEXT:   for (k ={{.*}}) {
  //          PRT-O-NEXT:     {{printf|TGT_PRINTF}}
  //          PRT-O-NEXT:     for (l ={{.*}})
  //          PRT-O-NEXT:       ;
  //          PRT-O-NEXT:   }
  //          PRT-O-NEXT: }
  //         PRT-OA-NEXT: // ---------OMP<-ACC--------
  //         PRT-OA-NEXT: {{^ *}}// #pragma acc loop
  //     PRT-OA-SRC-SAME: {{^ }}INDEPENDENT
  // PRT-OA-AST-IND-SAME: {{^ }}independent
  //         PRT-OA-SAME: {{^$}}
  //         PRT-OA-NEXT: // for (k ={{.*}}) {
  //         PRT-OA-NEXT: //   {{printf|TGT_PRINTF}}
  //         PRT-OA-NEXT: //   for (l ={{.*}})
  //         PRT-OA-NEXT: //     ;
  //         PRT-OA-NEXT: // }
  //         PRT-OA-NEXT: // ^----------ACC----------^
  //
  // PRT-NOACC-NEXT: for (k ={{.*}}) {
  // PRT-NOACC-NEXT:   {{printf|TGT_PRINTF}}
  // PRT-NOACC-NEXT:   for (l ={{.*}})
  // PRT-NOACC-NEXT:     ;
  // PRT-NOACC-NEXT: }
  #pragma acc loop INDEPENDENT
  for (k = 1; k < 3; ++k) {
    TGT_PRINTF("loop iteration\n");
    for (l = 0; l < 2; ++l)
      ;
  }
  // Each gang will see the old k from before the loop above.
  if (k != 999)
    save->assignLoopFoundBadVal = true;
  // Each gang will see the final l from the loop above.
  if (l != 2)
    save->assignNestedLoopFoundBadVal = true;

  // PRT: k = 999;
  k = 999;
  // k should have been dropped from the loop control variable list, so it
  // should now be implicitly shared in all cases.
  //
  //             DMP: ACCLoopDirective
  //    DMP-IND-NEXT:   ACCIndependentClause
  //     DMP-IND-NOT:     <implicit>
  // DMP-NO-IND-NEXT:   ACCIndependentClause {{.*}} <implicit>
  //        DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  //        DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
  //        DMP-NEXT:   impl: ForStmt
  //
  //          PRT-A-NEXT: {{^ *}}#pragma acc loop
  //      PRT-A-SRC-SAME: {{^ }}INDEPENDENT
  //  PRT-A-AST-IND-SAME: {{^ }}independent
  //         PRT-AO-SAME: {{^ }}// discarded in OpenMP translation
  //          PRT-A-SAME: {{^$}}
  //         PRT-OA-NEXT: {{^ *}}// #pragma acc loop
  //     PRT-OA-SRC-SAME: {{^ }}INDEPENDENT
  // PRT-OA-AST-IND-SAME: {{^ }}independent
  //         PRT-OA-SAME: {{^ }}// discarded in OpenMP translation{{$}}
  //            PRT-NEXT: for (int i = {{.*}}) {
  //            PRT-NEXT:   {{printf|TGT_PRINTF}}
  //            PRT-NEXT:   k = 9999;
  //            PRT-NEXT: }
  #pragma acc loop INDEPENDENT
  for (int i = 0; i < 2; ++i) {
    TGT_PRINTF("loop iteration\n");
    k = 9999;
  }
  // Each gang will see the k from the i loop above.
  if (k != 9999)
    save->writeInLaterLoopFoundBadVal = true;
}

// EXE-NOT: {{.}}
