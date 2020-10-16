//===--- ParseOpenACC.cpp - OpenACC directives parsing --------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
/// \file
///  This file implements parsing of all OpenACC directives and clauses.
///
//===----------------------------------------------------------------------===//

#include "clang/AST/ASTContext.h"
#include "clang/AST/StmtOpenACC.h"
#include "clang/Parse/ParseDiagnostic.h"
#include "clang/Parse/Parser.h"
#include "clang/Parse/RAIIObjectsForParser.h"
#include "clang/Sema/Scope.h"
#include "llvm/ADT/PointerIntPair.h"

using namespace clang;

namespace {
enum OpenACCDirectiveKindEx {
  ACCDEx_enter = ACCD_LAST_KIND + 1,
  ACCDEx_exit,
  ACCDEx_data
};

// Map token string to extended ACC token kind that are
// OpenACCDirectiveKind + OpenACCDirectiveKindEx.
unsigned getOpenACCDirectiveKindEx(StringRef S) {
  unsigned DKind = getOpenACCDirectiveKind(S);
  if (DKind != ACCD_unknown)
    return DKind;
  return llvm::StringSwitch<unsigned>(S)
      .Case("enter", ACCDEx_enter)
      .Case("exit", ACCDEx_exit)
      .Default(ACCD_unknown);
}

OpenACCDirectiveKind parseOpenACCDirectiveKind(Parser &P) {
  // Array of foldings: F[i][0] F[i][1] ===> F[i][2].
  // E.g.: ACCD_parallel ACCD_loop ===> ACCD_parallel_loop
  // TODO: add other combined directives in topological order.
  static const unsigned F[][3] = {
      {ACCD_parallel, ACCD_loop, ACCD_parallel_loop},
      {ACCDEx_enter, ACCD_data, ACCD_enter_data},
      {ACCDEx_exit, ACCD_data, ACCD_exit_data}};
  auto Tok = P.getCurToken();
  unsigned DKind =
      Tok.isAnnotation()
          ? static_cast<unsigned>(ACCD_unknown)
          : getOpenACCDirectiveKindEx(P.getPreprocessor().getSpelling(Tok));
  if (DKind == ACCD_unknown)
    return ACCD_unknown;

  for (unsigned I = 0; I < llvm::array_lengthof(F); ++I) {
    if (DKind != F[I][0])
      continue;

    Tok = P.getPreprocessor().LookAhead(0);
    unsigned SDKind =
        Tok.isAnnotation()
            ? static_cast<unsigned>(ACCD_unknown)
            : getOpenACCDirectiveKind(P.getPreprocessor().getSpelling(Tok));
    if (SDKind == ACCD_unknown)
      continue;

    if (SDKind == F[I][1]) {
      P.ConsumeToken();
      DKind = F[I][2];
    }
  }
  return DKind < ACCD_unknown ? static_cast<OpenACCDirectiveKind>(DKind)
                              : ACCD_unknown;
}
} // namespace

/// Parsing of declarative OpenACC directives.  None are supported yet, but
/// we want to give a better diagnostic than a syntax error.
///
Parser::DeclGroupPtrTy Parser::ParseOpenACCDeclarativeDirective() {
  assert(Tok.is(tok::annot_pragma_openacc) && "Not an OpenACC directive!");
  ParenBraceBracketBalancer BalancerRAIIObj(*this);

  ConsumeAnnotationToken();
  OpenACCDirectiveKind DKind = parseOpenACCDirectiveKind(*this);

  switch (DKind) {
  case ACCD_unknown:
    Diag(Tok, diag::err_acc_unknown_directive);
    break;
  case ACCD_update:
  case ACCD_enter_data:
  case ACCD_exit_data:
  case ACCD_data:
  case ACCD_parallel:
  case ACCD_loop:
  case ACCD_parallel_loop:
    Diag(Tok, diag::err_acc_unexpected_directive)
        << getOpenACCName(DKind);
    break;
  }
  while (Tok.isNot(tok::annot_pragma_openacc_end))
    ConsumeAnyToken();
  ConsumeAnyToken();
  return nullptr;
}

///  Parsing of declarative or executable OpenACC directives.
///
///       executable-directive:
///         annot_pragma_openacc
///         'update' | 'enter data' | 'exit data' | 'data' | 'parallel' | 'loop'
///         | 'parallel loop'
///         {clause}
///         annot_pragma_openacc_end
///
StmtResult Parser::ParseOpenACCDeclarativeOrExecutableDirective(
    ParsedStmtContext StmtCtx) {
  assert(Tok.is(tok::annot_pragma_openacc) && "Not an OpenACC directive!");
  ParenBraceBracketBalancer BalancerRAIIObj(*this);
  SmallVector<ACCClause *, 5> Clauses;
  SmallVector<bool, ACCC_unknown + 1> FirstClauses(ACCC_unknown + 1);
  unsigned ScopeFlags =
      Scope::FnScope | Scope::DeclScope | Scope::OpenACCDirectiveScope;
  SourceLocation Loc = ConsumeAnnotationToken(), EndLoc;
  OpenACCDirectiveKind DKind = parseOpenACCDirectiveKind(*this);
  StmtResult Directive = StmtError();
  bool HasAssociatedStatement = true;

  switch (DKind) {
  case ACCD_update:
  case ACCD_enter_data:
  case ACCD_exit_data:
    // OpenACC 3.0 sec. 2.14.4 "Update Directive" L2296-2298:
    //   "The update directive is executable.  It must not appear in place of
    //   the statement following an if, while, do, switch, or label in C or C++,
    //   or in place of the statement following a logical if in Fortran."
    // There appears to be no similar restriction for other OpenACC executable
    // directives.
    //
    // Clang does not follow this exactly.  Instead, Clang's OpenACC and OpenMP
    // support are consistent here, facilitating translation to OpenMP.  For
    // example, Clang does not enforce the case of labels, and Clang
    // additionally restricts the cases of "else", "for", and other directives.
    // More importantly, Clang enforces these restrictions for all executables
    // directives.
    if ((StmtCtx & ParsedStmtContext::AllowExecutableOpenACCDirectives) ==
        ParsedStmtContext()) {
      Diag(Tok, diag::err_acc_immediate_substatement) << getOpenACCName(DKind);
    }
    HasAssociatedStatement = false;
    LLVM_FALLTHROUGH;
  case ACCD_data:
  case ACCD_parallel:
  case ACCD_loop:
  case ACCD_parallel_loop: {
    ConsumeToken();

    if (isOpenACCLoopDirective(DKind))
      ScopeFlags |= Scope::OpenACCLoopDirectiveScope;
    ParseScope ACCDirectiveScope(this, ScopeFlags);
    Actions.StartOpenACCDABlock(DKind, Loc);

    while (Tok.isNot(tok::annot_pragma_openacc_end)) {
      OpenACCClauseKind CKind =
          Tok.isAnnotation()
              ? ACCC_unknown : getOpenACCClauseKind(PP.getSpelling(Tok));
      Actions.StartOpenACCClause(CKind);
      ACCClause *Clause = ParseOpenACCClause(DKind, CKind, FirstClauses);
      FirstClauses[CKind] = true;
      if (Clause)
        Clauses.push_back(Clause);

      // Skip ',' if any.
      if (Tok.is(tok::comma))
        ConsumeToken();
      Actions.EndOpenACCClause();
    }
    // End location of the directive.
    EndLoc = Tok.getLocation();
    // Consume final annot_pragma_openacc_end.
    ConsumeAnnotationToken();

    bool ErrorFound = Actions.ActOnOpenACCRegionStart(DKind, Clauses, Loc);
    StmtResult AssociatedStmt;
    if (HasAssociatedStatement)
      AssociatedStmt = ParseStatement();
    ErrorFound |= Actions.ActOnOpenACCRegionEnd();
    Directive = Actions.ActOnOpenACCExecutableDirective(
        DKind, Clauses, AssociatedStmt.get(), Loc, EndLoc);

    // Exit scope.
    Actions.EndOpenACCDABlock();
    ACCDirectiveScope.Exit();
    // Don't bother translating to OpenMP if we've encountered errors or we
    // might end up with redundant diagnostics, some of which might mention
    // OpenMP.
    if (!Actions.getDiagnostics().hasErrorOccurred()) {
      assert(!Directive.isInvalid()
             && "Invalid OpenACC directive without diagnostic");
      if (Actions.transformACCToOMP(cast<ACCExecutableDirective>(
              Directive.get())))
        ErrorFound = true;
    }
    if (ErrorFound)
      Directive = StmtError();
    break;
  }
  case ACCD_unknown:
    Diag(Tok, diag::err_acc_unknown_directive);
    SkipUntil(tok::annot_pragma_openacc_end);
    break;
  }
  return Directive;
}

///  Parsing of OpenACC clauses.
///
///    clause:
///       present | copy-clause | pcopy-clause | present_or_copy-clause
///       | copyin-clause | pcopyin-clause | present_or_copyin-clause
///       | copyout-clause | pcopyout-clause | present_or_copyout-clause
///       | create-clause | pcreate-clause | present_or_create-clause
///       | no_create-clause | delete-clause
///       | private-clause | firstprivate-clause | reduction-clause | if-clause
///       | if-present-clause | self-clause | host-clause | device-clause
///       | num_gangs-clause | num_workers-clause | vector_length-clause
///       | seq-clause | independent-clause | auto-clause
///       | gang-clause | worker-clause | vector-clause | collapse-clause
///
ACCClause *Parser::ParseOpenACCClause(
    OpenACCDirectiveKind DKind, OpenACCClauseKind CKind,
    const SmallVectorImpl<bool> &FirstClauses) {
  ACCClause *Clause = nullptr;
  bool ErrorFound = false;
  bool WrongDirective = false;
  // Check if clause is allowed for the given directive.
  if (CKind != ACCC_unknown && !isAllowedClauseForDirective(DKind, CKind)) {
    Diag(Tok, diag::err_acc_unexpected_clause) << getOpenACCName(CKind)
                                               << getOpenACCName(DKind);
    ErrorFound = true;
    WrongDirective = true;
  }

  switch (CKind) {
  case ACCC_if:
  case ACCC_num_gangs:
  case ACCC_num_workers:
  case ACCC_vector_length:
  case ACCC_collapse:
    if (FirstClauses[CKind]) {
      Diag(Tok, diag::err_acc_more_one_clause)
          << getOpenACCName(DKind) << getOpenACCName(CKind);
      ErrorFound = true;
    }
    Clause = ParseOpenACCSingleExprClause(CKind, WrongDirective);
    break;
  case ACCC_if_present:
  case ACCC_seq:
  case ACCC_independent:
  case ACCC_auto:
  case ACCC_gang:
  case ACCC_worker:
  case ACCC_vector:
    if (FirstClauses[CKind]) {
      Diag(Tok, diag::err_acc_more_one_clause)
          << getOpenACCName(DKind) << getOpenACCName(CKind);
      ErrorFound = true;
    }
    else {
      if (CKind == ACCC_seq || CKind == ACCC_independent ||
           CKind == ACCC_auto) {
        OpenACCClauseKind CKindOther;
        if (FirstClauses[CKindOther = ACCC_seq] ||
            FirstClauses[CKindOther = ACCC_independent] ||
            FirstClauses[CKindOther = ACCC_auto]) {
          Diag(Tok, diag::err_acc_mutually_exclusive_clauses)
              << getOpenACCName(CKind) << getOpenACCName(CKindOther);
          ErrorFound = true;
        }
      }
      if (CKind == ACCC_seq) {
        OpenACCClauseKind CKindOther;
        if (FirstClauses[CKindOther = ACCC_gang] ||
            FirstClauses[CKindOther = ACCC_worker] ||
            FirstClauses[CKindOther = ACCC_vector]) {
          Diag(Tok, diag::err_acc_mutually_exclusive_clauses)
              << getOpenACCName(CKind) << getOpenACCName(CKindOther);
          ErrorFound = true;
        }
      }
      else if ((CKind == ACCC_gang || CKind == ACCC_worker ||
                CKind == ACCC_vector) &&
               FirstClauses[ACCC_seq]) {
        Diag(Tok, diag::err_acc_mutually_exclusive_clauses)
            << getOpenACCName(CKind) << getOpenACCName(ACCC_seq);
        ErrorFound = true;
      }
    }
    Clause = ParseOpenACCClause(CKind, WrongDirective);
    break;
  case ACCC_present:
#define OPENACC_CLAUSE_ALIAS_copy(Name) \
  case ACCC_##Name:
#define OPENACC_CLAUSE_ALIAS_copyin(Name) \
  case ACCC_##Name:
#define OPENACC_CLAUSE_ALIAS_copyout(Name) \
  case ACCC_##Name:
#define OPENACC_CLAUSE_ALIAS_create(Name) \
  case ACCC_##Name:
#include "clang/Basic/OpenACCKinds.def"
  case ACCC_no_create:
  case ACCC_delete:
  case ACCC_private:
  case ACCC_firstprivate:
  case ACCC_reduction:
#define OPENACC_CLAUSE_ALIAS_self(Name) \
  case ACCC_##Name:
#include "clang/Basic/OpenACCKinds.def"
  case ACCC_device:
    Clause = ParseOpenACCVarListClause(DKind, CKind, WrongDirective);
    break;
  case ACCC_nomap:
  case ACCC_shared:
    assert((ErrorFound && WrongDirective) &&
           "expected clause not to be permitted on a directive");
    // Discard any var list.
    ParseOpenACCVarListClause(DKind, CKind, WrongDirective);
    break;
  case ACCC_unknown:
    Diag(Tok, diag::warn_acc_extra_tokens_at_eol)
        << getOpenACCName(DKind);
    SkipUntil(tok::annot_pragma_openacc_end, StopBeforeMatch);
    break;
  }
  return ErrorFound ? nullptr : Clause;
}

/// Parses simple expression in parens for single-expression clauses of OpenACC
/// constructs.
/// \param RLoc Returned location of right paren.
ExprResult Parser::ParseOpenACCParensExpr(StringRef ClauseName,
                                          SourceLocation &RLoc) {
  BalancedDelimiterTracker T(*this, tok::l_paren, tok::annot_pragma_openacc_end);
  if (T.expectAndConsume(diag::err_expected_lparen_after, ClauseName.data()))
    return ExprError();

  SourceLocation ELoc = Tok.getLocation();
  ExprResult LHS(ParseCastExpression(AnyCastExpr, /*isAddressOfOperand=*/false,
                                     NotTypeCast));
  ExprResult Val(ParseRHSOfBinaryExpression(LHS, prec::Conditional));
  Val = Actions.ActOnFinishFullExpr(Val.get(), ELoc, /*DiscardedValue*/ false);

  // Parse ')'.
  RLoc = Tok.getLocation();
  if (!T.consumeClose())
    RLoc = T.getCloseLocation();

  return Val;
}

///  Parsing of OpenACC clauses with single expressions like 'num_gangs'.
///
///    num_gangs-clause:
///      'num_gangs' '(' expression ')'
///
ACCClause *Parser::ParseOpenACCSingleExprClause(OpenACCClauseKind Kind,
                                                bool ParseOnly) {
  SourceLocation Loc = ConsumeToken();
  SourceLocation LLoc = Tok.getLocation();
  SourceLocation RLoc;

  ExprResult Val = ParseOpenACCParensExpr(getOpenACCName(Kind), RLoc);

  if (Val.isInvalid())
    return nullptr;

  if (ParseOnly)
    return nullptr;
  return Actions.ActOnOpenACCSingleExprClause(Kind, Val.get(), Loc, LLoc, RLoc);
}

///  Parsing of OpenACC clauses like 'seq'.
///
///    seq-clause:
///         'seq'
///
ACCClause *Parser::ParseOpenACCClause(OpenACCClauseKind Kind, bool ParseOnly) {
  SourceLocation Loc = Tok.getLocation();
  ConsumeAnyToken();

  if (ParseOnly)
    return nullptr;
  return Actions.ActOnOpenACCClause(Kind, Loc, Tok.getLocation());
}

static bool ParseReductionId(Parser &P, UnqualifiedId &ReductionId) {
  auto OOK = OO_None;
  switch (P.getCurToken().getKind()) {
  case tok::plus:
    OOK = OO_Plus;
    break;
  case tok::star:
    OOK = OO_Star;
    break;
  case tok::amp:
    OOK = OO_Amp;
    break;
  case tok::pipe:
    OOK = OO_Pipe;
    break;
  case tok::caret:
    OOK = OO_Caret;
    break;
  case tok::ampamp:
    OOK = OO_AmpAmp;
    break;
  case tok::pipepipe:
    OOK = OO_PipePipe;
    break;
  default:
    break;
  }
  if (OOK != OO_None) {
    SourceLocation OpLoc = P.ConsumeToken();
    SourceLocation SymbolLocations[] = {OpLoc, OpLoc, SourceLocation()};
    ReductionId.setOperatorFunctionId(OpLoc, OOK, SymbolLocations);
    return false;
  }
  if (P.getCurToken().is(tok::identifier)) {
    // Consume the identifier.
    IdentifierInfo *Id = P.getCurToken().getIdentifierInfo();
    SourceLocation IdLoc = P.ConsumeToken();
    ReductionId.setIdentifier(Id, IdLoc);
    return false;
  }
  P.Diag(P.getCurToken(), diag::err_acc_expected_reduction_operator);
  return true;
}

/// Parses clauses with list.
bool Parser::ParseOpenACCVarList(OpenACCDirectiveKind DKind,
                                 OpenACCClauseKind Kind,
                                 SmallVectorImpl<Expr *> &Vars,
                                 SourceLocation &ColonLoc,
                                 SourceLocation &EndLoc,
                                 DeclarationNameInfo &ReductionId) {
  UnqualifiedId UnqualifiedReductionId;
  bool InvalidReductionId = false;

  // Parse '('.
  BalancedDelimiterTracker T(*this, tok::l_paren, tok::annot_pragma_openacc_end);
  if (T.expectAndConsume(diag::err_expected_lparen_after,
                         getOpenACCName(Kind)))
    return true;

  // Handle reduction-identifier for reduction clause.
  if (Kind == ACCC_reduction) {
    ColonProtectionRAIIObject ColonRAII(*this);
    InvalidReductionId = ParseReductionId(*this, UnqualifiedReductionId);
    if (InvalidReductionId) {
      SkipUntil(tok::colon, tok::r_paren, tok::annot_pragma_openacc_end,
                StopBeforeMatch);
    }
    if (Tok.is(tok::colon))
      ColonLoc = ConsumeToken();
    else
      Diag(Tok, diag::warn_pragma_expected_colon) << "reduction operator";
    if (!InvalidReductionId)
      ReductionId = Actions.GetNameFromUnqualifiedId(UnqualifiedReductionId);
  }

  bool IsComma =
      Kind != ACCC_reduction ||
      (Kind == ACCC_reduction && !InvalidReductionId);
  while (IsComma || (Tok.isNot(tok::r_paren) && Tok.isNot(tok::colon) &&
                     Tok.isNot(tok::annot_pragma_openacc_end))) {
    ColonProtectionRAIIObject ColonRAII(*this, false);
    // Parse variable
    ExprResult VarExpr =
        Actions.CorrectDelayedTyposInExpr(ParseAssignmentExpression());
    if (VarExpr.isUsable())
      Vars.push_back(VarExpr.get());
    else {
      SkipUntil(tok::comma, tok::r_paren, tok::annot_pragma_openacc_end,
                StopBeforeMatch);
    }
    // Skip ',' if any
    IsComma = Tok.is(tok::comma);
    if (IsComma)
      ConsumeToken();
    else if (Tok.isNot(tok::r_paren) &&
             Tok.isNot(tok::annot_pragma_openacc_end))
      Diag(Tok, diag::err_acc_expected_punc)
          << getOpenACCName(Kind);
  }

  // Parse ')'.
  EndLoc = Tok.getLocation();
  if (!T.consumeClose())
    EndLoc = T.getCloseLocation();
  return Vars.empty() || InvalidReductionId;
}

///  Parsing of OpenACC clause 'copy', 'pcopy', 'present_or_copy', 'copyin',
///  'pcopyin', 'present_or_copyin', 'copyout', 'pcopyout',
///  'present_or_copyout', 'private', 'firstprivate', or 'reduction'.
///
///    copy-clause:
///       'copy' '(' list ')'
///    pcopy-clause:
///       'pcopy' '(' list ')'
///    present_or_copy-clause:
///       'present_or_copy' '(' list ')'
///    copyin-clause:
///       'copyin' '(' list ')'
///    pcopyin-clause:
///       'pcopyin' '(' list ')'
///    present_or_copyin-clause:
///       'present_or_copyin' '(' list ')'
///    copyout-clause:
///       'copyout' '(' list ')'
///    pcopyout-clause:
///       'pcopyout' '(' list ')'
///    present_or_copyout-clause:
///       'present_or_copyout' '(' list ')'
///    private-clause:
///       'private' '(' list ')'
///    firstprivate-clause:
///       'firstprivate' '(' list ')'
///    reduction-clause:
///       'reduction' '(' reduction-identifier ':' list ')'
ACCClause *Parser::ParseOpenACCVarListClause(OpenACCDirectiveKind DKind,
                                             OpenACCClauseKind Kind,
                                             bool ParseOnly) {
  SourceLocation Loc = Tok.getLocation();
  SourceLocation LOpen = ConsumeToken();
  SmallVector<Expr *, 4> Vars;
  SourceLocation ColonLoc;
  SourceLocation EndLoc;
  DeclarationNameInfo ReductionId;

  if (ParseOpenACCVarList(DKind, Kind, Vars, ColonLoc, EndLoc, ReductionId))
    return nullptr;

  if (ParseOnly)
    return nullptr;
  return Actions.ActOnOpenACCVarListClause(Kind, Vars, Loc, LOpen, ColonLoc,
                                           EndLoc, ReductionId);
}
