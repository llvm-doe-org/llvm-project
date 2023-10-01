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

  for (const auto &I : F) {
    if (DKind != I[0])
      continue;

    Tok = P.getPreprocessor().LookAhead(0);
    unsigned SDKind =
        Tok.isAnnotation()
            ? static_cast<unsigned>(ACCD_unknown)
            : getOpenACCDirectiveKind(P.getPreprocessor().getSpelling(Tok));
    if (SDKind == ACCD_unknown)
      continue;

    if (SDKind == I[1]) {
      P.ConsumeToken();
      DKind = I[2];
    }
  }
  return DKind < ACCD_unknown ? static_cast<OpenACCDirectiveKind>(DKind)
                              : ACCD_unknown;
}
} // namespace

void Parser::ParseOpenACCClauses(OpenACCDirectiveKind DKind) {
  SmallVector<bool, ACCC_unknown + 1> SeenClauses(ACCC_unknown + 1);
  ConsumeToken();
  while (Tok.isNot(tok::annot_pragma_openacc_end)) {
    OpenACCClauseKind CKind = Tok.isAnnotation()
                                  ? ACCC_unknown
                                  : getOpenACCClauseKind(PP.getSpelling(Tok));
    ACCClause *Clause = ParseOpenACCClause(DKind, CKind, SeenClauses);
    if (Clause)
      Actions.AddOpenACCClause(Clause);
    // Skip ',' if any.
    if (Tok.is(tok::comma))
      ConsumeToken();
  }
}

/// Parsing of OpenACC declarative directives.
///
///   routine-directive:
///     annot_pragma_openacc 'routine' 'seq' annot_pragma_openacc_end
///     <function declaration/definition>
Parser::DeclGroupPtrTy
Parser::ParseOpenACCDeclarativeDirective(DeclaratorContext Context,
                                         ParsedAttributes &DeclAttrs,
                                         ParsedAttributes &DeclSpecAttrs) {
  assert(Tok.is(tok::annot_pragma_openacc) && "expected an OpenACC directive");
  SourceLocation StartLoc = ConsumeAnnotationToken();
  OpenACCDirectiveKind DKind = parseOpenACCDirectiveKind(*this);
  SourceLocation EndLoc;
  return ParseOpenACCDeclarativeDirective(Context, DeclAttrs, DeclSpecAttrs,
                                          StartLoc, DKind, EndLoc);
}
Parser::DeclGroupPtrTy Parser::ParseOpenACCDeclarativeDirective(
    DeclaratorContext Context, ParsedAttributes &DeclAttrs,
    ParsedAttributes &DeclSpecAttrs, SourceLocation StartLoc,
    OpenACCDirectiveKind DKind, SourceLocation &EndLoc) {
  return ParseOpenACCDeclarativeDirective(
      Context, DeclAttrs, DeclSpecAttrs, AS_none, DeclSpec::TST_unspecified,
      /*Tag=*/nullptr, StartLoc, DKind, EndLoc);
}
Parser::DeclGroupPtrTy
Parser::ParseOpenACCDeclarativeDirective(ParsedAttributes &AccessAttrs,
                                         AccessSpecifier AS,
                                         DeclSpec::TST TagType, Decl *Tag) {
  assert(Tok.is(tok::annot_pragma_openacc) && "expected an OpenACC directive");
  SourceLocation StartLoc = ConsumeAnnotationToken();
  OpenACCDirectiveKind DKind = parseOpenACCDirectiveKind(*this);
  SourceLocation EndLoc;
  ParsedAttributes EmptyAttrs(getAttrFactory());
  return ParseOpenACCDeclarativeDirective(DeclaratorContext::Member,
                                          AccessAttrs, EmptyAttrs, AS, TagType,
                                          Tag, StartLoc, DKind, EndLoc);
}
Parser::DeclGroupPtrTy Parser::ParseOpenACCDeclarativeDirective(
    DeclaratorContext Context, ParsedAttributes &DeclAttrs,
    ParsedAttributes &DeclSpecAttrs, AccessSpecifier AS, DeclSpec::TST TagType,
    Decl *Tag, SourceLocation StartLoc, OpenACCDirectiveKind DKind,
    SourceLocation &EndLoc) {
  ParenBraceBracketBalancer BalancerRAIIObj(*this);
  SmallVector<ACCClause *, 5> Clauses;
  switch (DKind) {
  case ACCD_update:
  case ACCD_enter_data:
  case ACCD_exit_data:
  case ACCD_wait:
  case ACCD_data:
  case ACCD_parallel:
  case ACCD_loop:
  case ACCD_parallel_loop:
  case ACCD_atomic:
    Diag(Tok, diag::err_acc_unexpected_directive)
        << getOpenACCName(DKind);
    SkipUntil(tok::annot_pragma_openacc_end);
    break;
  case ACCD_routine: {
    if (!Actions.getCurLexicalContext()->isFileContext() &&
        (!Actions.getCurLexicalContext()->isRecord() || AS == AS_none) &&
        !Actions.getCurLexicalContext()->isFunctionOrMethod()) {
      Diag(StartLoc, diag::err_acc_unexpected_directive)
          << getOpenACCName(DKind);
      SkipUntil(tok::annot_pragma_openacc_end);
      return nullptr;
    }
    if (Actions.StartOpenACCDirectiveAndAssociate(DKind, StartLoc)) {
      SkipUntil(tok::annot_pragma_openacc_end);
      Actions.EndOpenACCDirectiveAndAssociate(DKind);
      return nullptr;
    }
    ParseOpenACCClauses(DKind);
    Actions.EndOpenACCDirective(Tok.getLocation());
    // Consume final annot_pragma_openacc_end.
    ConsumeAnnotationToken();
    DeclGroupPtrTy Group;
    // Parse the function declaration.
    //
    // If another OpenACC directive follows immediately, that's not an
    // immediately following function declaration, so don't try to parse it as
    // one.  Instead complain that there's no function declaration.  That's
    // particularly important if that following directive is a routine
    // directive.  If we tried to parse that as a function declaration, we'd be
    // able to accept a series of routine directives as if they all apply to a
    // single function declaration following them all.
    //
    // As a special error recovery case, if the next token is ';', '{', or '}'
    // (so that the routine directive was mistakenly used syntactically like a
    // compute construct or an executable directive), don't try to parse a
    // declaration because the parser would produce redundant complaints about
    // the ';' or discard the '{' or '}' and then potentially get confused
    // thinking braces are unbalanced.  Instead, just leave the ';', '{', or
    // '}', complain there's no function declaration, and let the parser later
    // complain about the '{' or '}' if it has no match.
    //
    // Likewise, if we're at EOF, we don't need two complaints that there's no
    // declaration, so don't try to parse one, and complain once that there
    // isn't one.
    if (!isEofOrEom() && !Tok.isOneOf(tok::annot_pragma_openacc, tok::l_brace,
                                      tok::r_brace, tok::semi)) {
      if (AS == AS_none) {
        Group = ParseDeclaration(Context, EndLoc, DeclAttrs, DeclSpecAttrs);
      } else {
        // TODO: Recombining the attributes here seems fishy.  What is right?
        ParsedAttributes Attrs(AttrFactory);
        takeAndConcatenateAttrs(DeclAttrs, DeclSpecAttrs, Attrs);
        Group =
            ParseCXXClassMemberDeclarationWithPragmas(AS, Attrs, TagType, Tag);
      }
    }
    Actions.ActOnOpenACCRoutineDirective(ACC_EXPLICIT, Group.get());
    Actions.EndOpenACCDirectiveAndAssociate(DKind);
    return Group;
  }
  case ACCD_unknown:
    Diag(Tok, diag::err_acc_unknown_directive);
    SkipUntil(tok::annot_pragma_openacc_end);
    break;
  }
  return nullptr;
}

/// Parsing of OpenACC executable directives and constructs.
///
///   directive-stmt:
///     annot_pragma_openacc
///     'update' | 'enter data' | 'exit data' | 'wait' | 'data' | 'parallel'
///     | 'loop' | 'parallel loop' | 'atomic'
///     {clause}
///     annot_pragma_openacc_end
///
/// Keep TreeTransform::TransformACCDirectiveStmt in sync with this
/// implementation.
StmtResult Parser::ParseOpenACCDirectiveStmt(ParsedAttributes &DeclAttrs,
                                             ParsedAttributes &DeclSpecAttrs,
                                             ParsedStmtContext StmtCtx) {
  assert(Tok.is(tok::annot_pragma_openacc) && "Not an OpenACC directive!");
  ParenBraceBracketBalancer BalancerRAIIObj(*this);
  unsigned ScopeFlags =
      Scope::FnScope | Scope::DeclScope | Scope::OpenACCDirectiveScope;
  SourceLocation StartLoc = ConsumeAnnotationToken();
  OpenACCDirectiveKind DKind = parseOpenACCDirectiveKind(*this);
  StmtResult Result = StmtError();
  bool HasAssociatedStatement = true;

  switch (DKind) {
  case ACCD_update:
  case ACCD_enter_data:
  case ACCD_exit_data:
  case ACCD_wait:
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
    // More importantly, Clang enforces these restrictions for all executable
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
  case ACCD_parallel_loop:
  case ACCD_atomic: {
    ProhibitAttributes(DeclAttrs);
    ProhibitAttributes(DeclSpecAttrs);
    if (isOpenACCLoopDirective(DKind))
      ScopeFlags |= Scope::OpenACCLoopDirectiveScope;
    ParseScope ACCDirectiveScope(this, ScopeFlags);
    bool ErrorFound =
        Actions.StartOpenACCDirectiveAndAssociate(DKind, StartLoc);

    ParseOpenACCClauses(DKind);
    Actions.EndOpenACCDirective(Tok.getLocation());
    // Consume final annot_pragma_openacc_end.
    ConsumeAnnotationToken();

    ErrorFound |= Actions.StartOpenACCAssociatedStatement();
    StmtResult AssociatedStmt;
    if (HasAssociatedStatement)
      AssociatedStmt = ParseStatement();
    ErrorFound |= Actions.EndOpenACCAssociatedStatement();
    Result = Actions.ActOnOpenACCDirectiveStmt(AssociatedStmt.get());

    // Exit scope.
    Actions.EndOpenACCDirectiveAndAssociate(DKind);
    ACCDirectiveScope.Exit();
    if (!Result.isInvalid() &&
        Actions.transformACCToOMP(cast<ACCDirectiveStmt>(Result.get())))
      ErrorFound = true;
    if (ErrorFound)
      Result = StmtError();
    break;
  }
  case ACCD_routine: {
    SourceLocation EndLoc;
    DeclGroupPtrTy Group = ParseOpenACCDeclarativeDirective(
        DeclaratorContext::Block, DeclAttrs, DeclSpecAttrs, StartLoc, DKind,
        EndLoc);
    if (Group)
      Result = Actions.ActOnDeclStmt(Group, StartLoc, EndLoc);
    break;
  }
  case ACCD_unknown:
    Diag(Tok, diag::err_acc_unknown_directive);
    SkipUntil(tok::annot_pragma_openacc_end);
    break;
  }
  return Result;
}

///  Parsing of OpenACC clauses.
///
///    clause:
///       present-clause | copy-clause | pcopy-clause | present_or_copy-clause
///       | copyin-clause | pcopyin-clause | present_or_copyin-clause
///       | copyout-clause | pcopyout-clause | present_or_copyout-clause
///       | create-clause | pcreate-clause | present_or_create-clause
///       | no_create-clause | delete-clause
///       | private-clause | firstprivate-clause | reduction-clause | if-clause
///       | if-present-clause | self-clause | host-clause | device-clause
///       | num_gangs-clause | num_workers-clause | vector_length-clause
///       | seq-clause | independent-clause | auto-clause
///       | gang-clause | worker-clause | vector-clause | collapse-clause
///       | tile-clause | async-clause | wait-clause
///       | read-clause | write-clause | update-clause | capture-clause
///
ACCClause *Parser::ParseOpenACCClause(OpenACCDirectiveKind DKind,
                                      OpenACCClauseKind CKind,
                                      SmallVectorImpl<bool> &SeenClauses) {
  ACCClause *Clause = nullptr;
  bool ErrorFound = false;
  bool WrongDirective = false;

  // Check if the clause is ever permitted for the given directive.  If it's
  // not, we finish parsing it based on its syntax in other directives because
  // that's likely how the programmer wrote it and so that's probably the best
  // way to reach and diagnose issues with subsequent clauses.  However, the
  // clause has no meaning on this directive, so diagnostics about its conflicts
  // with other occurrences of itself or with other clauses would be baseless
  // and confusing, so we suppress them.
  if ((CKind != ACCC_unknown && !isAllowedClauseForDirective(DKind, CKind)) ||
      (!Actions.getLangOpts().OpenACCFakeAsyncWait &&
       (CKind == ACCC_wait ||
        (CKind == ACCC_async && !isOpenACCParallelDirective(DKind))))) {
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
  case ACCC_async: {
    if (!WrongDirective) {
      if (SeenClauses[CKind]) {
        Diag(Tok, diag::err_acc_more_one_clause)
            << getOpenACCName(DKind) << getOpenACCName(CKind);
        ErrorFound = true;
      } else {
        checkMutuallyExclusiveClauses(SeenClauses, CKind, ErrorFound,
                                      {ACCC_collapse, ACCC_tile});
      }
    }
    NamedDecl *Async2Dep = nullptr;
    if (CKind == ACCC_async) {
      // Sema::getCurScope's documentation says it's safe to call during parsing
      // but not in any code that might be called during C++ template
      // instantiation, like Sema actions.  Thus, we perform this lookup here
      // and pass it through to Sema.
      DeclarationName Async2DepDN(
          &Actions.getASTContext().Idents.get(ACCAsyncClause::Async2DepName));
      Async2Dep = Actions.LookupSingleName(getCurScope(), Async2DepDN,
                                           Tok.getLocation(),
                                           Sema::LookupOrdinaryName);
    }
    if (CKind == ACCC_async && PP.LookAhead(/*N=*/0).isNot(tok::l_paren))
      Clause = ParseOpenACCNoArgClause(CKind, WrongDirective, Async2Dep);
    else
      Clause = ParseOpenACCSingleExprClause(CKind, WrongDirective, Async2Dep);
    break;
  }
  case ACCC_wait:
    if (!WrongDirective && SeenClauses[CKind]) {
      Diag(Tok, diag::err_acc_more_one_clause)
          << getOpenACCName(DKind) << getOpenACCName(CKind);
      ErrorFound = true;
    }
    if (PP.LookAhead(/*N=*/0).isNot(tok::l_paren))
      Clause = ParseOpenACCNoArgClause(CKind, WrongDirective,
                                       /*Async2Dep=*/nullptr);
    else
      Clause = ParseOpenACCWaitClauseWithArg(WrongDirective);
    break;
  case ACCC_if_present:
  case ACCC_seq:
  case ACCC_independent:
  case ACCC_auto:
  case ACCC_gang:
  case ACCC_worker:
  case ACCC_vector:
    if (!WrongDirective) {
      if (SeenClauses[CKind]) {
        Diag(Tok, diag::err_acc_more_one_clause)
            << getOpenACCName(DKind) << getOpenACCName(CKind);
        ErrorFound = true;
      } else if (DKind == ACCD_routine) {
        // OpenACC 3.2, sec. 2.15.1 "Routine Directive", L2881, L2890-2891,
        // L2901-2902, and L2906-2907:
        // "Only one of the gang, worker, vector, and seq clauses may appear for
        // each device type."
        checkMutuallyExclusiveClauses(
            SeenClauses, CKind, ErrorFound,
            {ACCC_gang, ACCC_worker, ACCC_vector, ACCC_seq});
      } else {
        checkMutuallyExclusiveClauses(SeenClauses, CKind, ErrorFound,
                                      {ACCC_seq, ACCC_independent, ACCC_auto});
        checkMutuallyExclusiveClauses(SeenClauses, CKind, ErrorFound,
                                      {ACCC_gang, ACCC_worker, ACCC_vector},
                                      {ACCC_seq});
      }
    }
    if (isOpenACCLoopDirective(DKind) && CKind == ACCC_gang &&
        PP.LookAhead(/*N=*/0).is(tok::l_paren))
      Clause = ParseOpenACCGangClauseWithArg(WrongDirective);
    else
      Clause = ParseOpenACCNoArgClause(CKind, WrongDirective,
                                       /*Async2Dep=*/nullptr);
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
  case ACCC_tile:
    if (!WrongDirective) {
      if (SeenClauses[CKind]) {
        Diag(Tok, diag::err_acc_more_one_clause)
            << getOpenACCName(DKind) << getOpenACCName(CKind);
        ErrorFound = true;
      } else {
        checkMutuallyExclusiveClauses(SeenClauses, CKind, ErrorFound,
                                      {ACCC_collapse, ACCC_tile});
      }
    }
    Clause = ParseOpenACCTileClause(WrongDirective);
    break;
  case ACCC_read:
  case ACCC_write:
  case ACCC_update:
  case ACCC_capture:
  case ACCC_compare:
    if (!WrongDirective) {
      if (SeenClauses[CKind]) {
        Diag(Tok, diag::err_acc_more_one_clause)
            << getOpenACCName(DKind) << getOpenACCName(CKind);
        ErrorFound = true;
      } else {
        checkMutuallyExclusiveClauses(
            SeenClauses, CKind, ErrorFound,
            {ACCC_read, ACCC_write, ACCC_update, ACCC_capture, ACCC_compare});
      }
    }
    Clause = ParseOpenACCNoArgClause(CKind, WrongDirective,
                                     /*Async2Dep=*/nullptr);
    break;
  case ACCC_unknown:
    Diag(Tok, diag::warn_acc_extra_tokens_at_eol) << getOpenACCName(DKind);
    SkipUntil(tok::annot_pragma_openacc_end, StopBeforeMatch);
    break;
  }
  // If the clause is ever permitted on this directive, then record its
  // appearance for the sake of detecting conflicts with later clauses.
  if (!WrongDirective)
    SeenClauses[CKind] = true;
  return ErrorFound ? nullptr : Clause;
}

/// Parses simple expression for argument of OpenACC directive.
ExprResult Parser::ParseOpenACCArgExpr() {
  SourceLocation ExprLoc = Tok.getLocation();
  ExprResult LHS(ParseCastExpression(AnyCastExpr, /*isAddressOfOperand=*/false,
                                     NotTypeCast));
  ExprResult Val(ParseRHSOfBinaryExpression(LHS, prec::Conditional));
  return Actions.ActOnFinishFullExpr(Val.get(), ExprLoc,
                                     /*DiscardedValue=*/false);
}

/// Parses simple expression in parens for single-expression clause of OpenACC
/// directive.
/// \param RParenLoc Returns location of right paren.
ExprResult Parser::ParseOpenACCParensExpr(StringRef ClauseName,
                                          SourceLocation &RParenLoc) {
  // Parse '('.
  BalancedDelimiterTracker T(*this, tok::l_paren, tok::annot_pragma_openacc_end);
  if (T.expectAndConsume(diag::err_expected_lparen_after, ClauseName.data()))
    return ExprError();

  // Parse expression.
  ExprResult Val = Parser::ParseOpenACCArgExpr();

  // Parse ')'.
  RParenLoc = Tok.getLocation();
  if (!T.consumeClose())
    RParenLoc = T.getCloseLocation();

  return Val;
}

///  Parsing of OpenACC clauses with single expressions like 'num_gangs'.
///
///    num_gangs-clause:
///      'num_gangs' '(' expression ')'
///
ACCClause *Parser::ParseOpenACCSingleExprClause(OpenACCClauseKind Kind,
                                                bool ParseOnly,
                                                NamedDecl *Async2Dep) {
  SourceLocation Loc = ConsumeToken();
  SourceLocation LLoc = Tok.getLocation();
  SourceLocation RLoc;

  ExprResult Val = ParseOpenACCParensExpr(getOpenACCName(Kind), RLoc);

  if (Val.isInvalid())
    return nullptr;

  if (ParseOnly)
    return nullptr;
  return Actions.ActOnOpenACCSingleExprClause(Kind, Val.get(), Loc, LLoc, RLoc,
                                              Async2Dep);
}

/// Parsing of OpenACC 'gang' clause's argument.
///
///   gang-clause:
///     'gang' '(' 'static' ':' expression ')'
///
/// Fail an assert if next two tokens aren't 'gang' and '('.
///
/// If anything after that isn't as expected, diagnose it, and skip the rest of
/// the clause because we don't know what to expect next.  In the future, when
/// we support more 'gang' clause arguments, we can probably just skip until the
/// next argument (delimited by ',') instead.
ACCClause *Parser::ParseOpenACCGangClauseWithArg(bool ParseOnly) {
  // Consume 'gang'.
  StringRef ClauseName = getOpenACCName(ACCC_gang);
  SourceLocation StartLoc = ConsumeToken();

  // Consume '('.
  SourceLocation LParenLoc = Tok.getLocation();
  BalancedDelimiterTracker T(*this, tok::l_paren,
                             tok::annot_pragma_openacc_end);
#ifndef NDEBUG
  bool NoLParen =
#endif
      T.expectAndConsume(diag::err_expected_lparen_after, ClauseName.data());
  assert(!NoLParen && "expected ParseOpenACCGangClauseWithArg upon 'gang('");

  // Parse 'static'.
  SourceLocation StaticKwLoc;
  if (Tok.is(tok::kw_static))
    StaticKwLoc = ConsumeToken();
  else {
    Diag(Tok, diag::err_acc_clause_expected_token)
        << getOpenACCName(ACCC_gang) << getKeywordSpelling(tok::kw_static);
    SkipUntil(tok::r_paren, tok::annot_pragma_openacc_end, StopBeforeMatch);
    T.consumeClose();
    return nullptr;
  }

  // Parse ':'.
  SourceLocation StaticColonLoc;
  if (Tok.is(tok::colon))
    StaticColonLoc = ConsumeToken();
  else {
    Diag(Tok, diag::err_acc_clause_expected_token)
        << getOpenACCName(ACCC_gang) << getPunctuatorSpelling(tok::colon);
    SkipUntil(tok::r_paren, tok::annot_pragma_openacc_end, StopBeforeMatch);
    T.consumeClose();
    return nullptr;
  }

  // Parse arg for 'static:'.
  SourceLocation StaticArgLoc = Tok.getLocation();
  ExprResult StaticArg;
  if (Tok.is(tok::star)) {
    ConsumeToken();
    StaticArg = Actions.ActOnOpenACCStarExpr(StaticArgLoc);
  } else {
    StaticArg = ParseOpenACCArgExpr();
  }
  if (StaticArg.isInvalid()) {
    SkipUntil(tok::r_paren, tok::annot_pragma_openacc_end, StopBeforeMatch);
    T.consumeClose();
    return nullptr;
  }

  // Parse ')'.
  SourceLocation EndLoc = Tok.getLocation();
  if (T.consumeClose()) {
    SkipUntil(tok::annot_pragma_openacc_end, StopBeforeMatch);
    return nullptr;
  }

  // Build clause's node.
  if (ParseOnly)
    return nullptr;
  return Actions.ActOnOpenACCGangClause(StartLoc, LParenLoc, StaticKwLoc,
                                        StaticColonLoc, StaticArg.get(),
                                        EndLoc);
}

///  Parsing of OpenACC clauses like 'seq'.
///
///    seq-clause:
///         'seq'
///
ACCClause *Parser::ParseOpenACCNoArgClause(OpenACCClauseKind Kind,
                                           bool ParseOnly,
                                           NamedDecl *Async2Dep) {
  SourceLocation Loc = Tok.getLocation();
  ConsumeToken();

  if (ParseOnly)
    return nullptr;
  return Actions.ActOnOpenACCClause(Kind, Loc, Tok.getLocation(), Async2Dep);
}

void Parser::checkMutuallyExclusiveClauses(
    const SmallVectorImpl<bool> &SeenClauses, OpenACCClauseKind Kind,
    bool &ErrorFound, const SmallVector<OpenACCClauseKind> &List0,
    const SmallVector<OpenACCClauseKind> &List1) {
  for (int I = 0; I < 2; ++I) {
    const SmallVector<OpenACCClauseKind> &ListThis = I == 0 ? List0 : List1;
    const SmallVector<OpenACCClauseKind> &ListOther = I == 0 ? List1 : List0;
    for (OpenACCClauseKind KindThis : ListThis) {
      if (Kind == KindThis) {
        for (OpenACCClauseKind KindOther :
             ListOther.empty() ? ListThis : ListOther) {
          if (Kind != KindOther && SeenClauses[KindOther]) {
            Diag(Tok, diag::err_acc_mutually_exclusive_clauses)
                << getOpenACCName(Kind) << getOpenACCName(KindOther);
            ErrorFound = true;
          }
        }
        return;
      }
    }
  }
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
      Diag(Tok, diag::err_acc_clause_expected_comma_or_paren)
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

///  Parsing of OpenACC clause 'tile'.
///
///    tile-clause:
///       'tile' '(' list ')'
ACCClause *Parser::ParseOpenACCTileClause(bool ParseOnly) {
  SourceLocation StartLoc = Tok.getLocation();
  SmallVector<Expr *, 4> SizeExprs;
  SourceLocation EndLoc;

  // Parse '('.
  SourceLocation LParenLoc = ConsumeToken();
  BalancedDelimiterTracker T(*this, tok::l_paren,
                             tok::annot_pragma_openacc_end);
  if (T.expectAndConsume(diag::err_expected_lparen_after,
                         getOpenACCName(ACCC_tile)))
    return nullptr;

  bool IsComma = true;
  while (IsComma || (Tok.isNot(tok::r_paren) &&
                     Tok.isNot(tok::annot_pragma_openacc_end))) {
    // Parse size expression.
    ExprResult SizeExpr;
    if (Tok.is(tok::star))
      SizeExpr = Actions.ActOnOpenACCStarExpr(ConsumeToken());
    else
      SizeExpr = ParseOpenACCArgExpr();
    if (SizeExpr.isUsable())
      SizeExprs.push_back(SizeExpr.get());
    else
      SkipUntil(tok::comma, tok::r_paren, tok::annot_pragma_openacc_end,
                StopBeforeMatch);

    // Skip ',' if any
    IsComma = Tok.is(tok::comma);
    if (IsComma)
      ConsumeToken();
    else if (Tok.isNot(tok::r_paren) &&
             Tok.isNot(tok::annot_pragma_openacc_end))
      Diag(Tok, diag::err_acc_clause_expected_comma_or_paren)
          << getOpenACCName(ACCC_tile);
  }

  // Parse ')'.
  EndLoc = Tok.getLocation();
  if (!T.consumeClose())
    EndLoc = T.getCloseLocation();

  if (ParseOnly)
    return nullptr;
  return Actions.ActOnOpenACCTileClause(SizeExprs, StartLoc, LParenLoc, EndLoc);
}

/// Parsing of OpenACC clause 'wait' with an argument.
///
///   wait-clause:
///      'wait' '(' list ')'
ACCClause *Parser::ParseOpenACCWaitClauseWithArg(bool ParseOnly) {
  SourceLocation StartLoc = Tok.getLocation();
  SmallVector<Expr *, 4> QueueExprs;
  SourceLocation EndLoc;

  // Parse '('.
  SourceLocation LParenLoc = ConsumeToken();
  BalancedDelimiterTracker T(*this, tok::l_paren,
                             tok::annot_pragma_openacc_end);
  if (T.expectAndConsume(diag::err_expected_lparen_after,
                         getOpenACCName(ACCC_wait)))
    return nullptr;

  bool IsComma = true;
  while (IsComma || (Tok.isNot(tok::r_paren) &&
                     Tok.isNot(tok::annot_pragma_openacc_end))) {
    // Parse queue expression.
    ExprResult Val = ParseOpenACCArgExpr();
    if (Val.isUsable())
      QueueExprs.push_back(Val.get());
    else
      SkipUntil(tok::comma, tok::r_paren, tok::annot_pragma_openacc_end,
                StopBeforeMatch);

    // Skip ',' if any
    IsComma = Tok.is(tok::comma);
    if (IsComma)
      ConsumeToken();
    else if (Tok.isNot(tok::r_paren) &&
             Tok.isNot(tok::annot_pragma_openacc_end))
      Diag(Tok, diag::err_acc_clause_expected_comma_or_paren)
          << getOpenACCName(ACCC_wait);
  }

  // Parse ')'.
  EndLoc = Tok.getLocation();
  if (!T.consumeClose())
    EndLoc = T.getCloseLocation();

  if (ParseOnly)
    return nullptr;
  return Actions.ActOnOpenACCWaitClause(QueueExprs, StartLoc, LParenLoc,
                                        EndLoc);
}
